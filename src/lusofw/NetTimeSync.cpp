#if defined(ENABLE_NETWORK_TIME)

#include "NetTimeSync.h"

#include <Mesh.h>
#include <RTClib.h>
#include <string.h>
#include <helpers/AdvertDataHelpers.h>

// Trusted network time source identity. Only adverts signed by this Ed25519
// key are honoured as a time source. The advert signature (covers pubkey +
// timestamp + appdata) authenticates the packet, but cannot by itself stop
// REPLAY of a previously captured, validly-signed advert -- that is what the
// high-water mark below is for.
static const uint8_t TIMEKEEPER_IDENTITY[PUB_KEY_SIZE] = {
  0x01, 0xB2, 0xF5, 0xDA, 0x46, 0xBC, 0x0A, 0x9C, 0x67, 0xFB, 0x8E, 0xDC, 0x36, 0x62, 0x57, 0xB6,
  0x04, 0x52, 0x73, 0xB8, 0x9F, 0x37, 0xF3, 0x08, 0x04, 0x4A, 0xD5, 0x57, 0x17, 0x34, 0xD4, 0x62
};

static const uint32_t MIN_PLAUSIBLE_TS = 1767225600;   // 1 Jan 2026
static const int MAX_TIMEKEEPER_HOPS = 10;

static uint32_t ram_mark = 0;   // highest ts ACCEPTED since boot

/**
 * \brief  Is a timestamp from the (already authenticated) trusted timekeeper fresh?
 */
static bool isFresh(uint32_t ts) {
  if (ram_mark != 0 && ts <= ram_mark) return false;   // replay (or stale)
  if (ram_mark != 0 && ts > ram_mark + NetTimeSync::MAX_FORWARD_SECS) return false;   // absurd forward jump
  return true;
}

/**
 * \brief  Record that ts from the trusted timekeeper was accepted and applied.
 */
static void accepted(uint32_t ts) {
  ram_mark = ts;
}

bool NetTimeSync::handleTimekeeperAdvert(const mesh::Identity& id, uint32_t timestamp,
                                         const uint8_t* app_data, size_t app_data_len,
                                         int path_len, mesh::RTCClock& clk) {
  // Reject implausible timestamps (anything before year 2026) and only trust
  // time sources heard within 10 hops (limits propagation skew/abuse).
  if (timestamp < MIN_PLAUSIBLE_TS || path_len >= MAX_TIMEKEEPER_HOPS) return false;

  AdvertDataParser parser(app_data, app_data_len);
  if (!(parser.isValid() && parser.getType() == ADV_TYPE_NONE)) return false;

  if (memcmp(id.pub_key, TIMEKEEPER_IDENTITY, PUB_KEY_SIZE) != 0) {
    MESH_DEBUG_PRINTLN("Network time: invalid ID [%02X%02X], ignoring timestamp",
                       id.pub_key[0], id.pub_key[1]);
    return false;
  }

  // Freshness is judged against the TIMEKEEPER's clock (high-water mark),
  // never our own: a timestamp is accepted iff it is strictly newer than the
  // last one accepted.
  if (!isFresh(timestamp)) {
    MESH_DEBUG_PRINTLN("Network time: stale/REPLAY rejected (ts=%u <= high-water=%u)",
                       timestamp, ram_mark);
    return false;
  }

  int32_t diff = (int32_t)timestamp - (int32_t)clk.getCurrentTime();
  clk.setCurrentTime(timestamp);   // apply, forward or backward
  accepted(timestamp);
  DateTime dt = DateTime(timestamp);
  MESH_DEBUG_PRINTLN("Network time: applied, diff=%d sec -> %02d:%02d:%02d %d/%d/%d",
                     diff, dt.hour(), dt.minute(), dt.second(), dt.day(), dt.month(), dt.year());
  return true;
}

uint32_t NetTimeSync::getHighWater() {
  return ram_mark;
}

#endif // ENABLE_NETWORK_TIME
