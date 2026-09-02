#pragma once

#if defined(ENABLE_ADVERT_PROTECT)

#include <Mesh.h>
#include <string.h>

/*
 * Advert protection policy for lusofw (ENABLE_ADVERT_PROTECT).
 * Fully static: no instance state, no changes to the embedding class beyond
 * one call site (see examples/simple_repeater/MyMesh.cpp).
 *
 * The first advert heard from a given remote REPEATER is allowed to be
 * repeated by us; any further advert from the same origin (public key) is a
 * duplicate and is not repeated until 12h have passed since the last allowed
 * one. Suppressed adverts are still processed locally (contacts, neighbours,
 * network time) -- the caller just marks them do-not-retransmit.
 *
 * All state is RAM-only: it resets on reboot (like the neighbours list and
 * NetTimeSync's high-water mark), so the first advert heard after a reboot is
 * repeated once and the window re-arms.
 *
 * The window is armed at ACCEPT time: an advert we allow but that is later
 * dropped by allowPacketForward() (forwarding disabled, hop limit) still
 * starts a new window. Accepted to keep a single call site; it self-corrects
 * at the next window.
 */
class AdvertProtection {
public:
  // Minimum time between repeats, by us, of adverts from the same repeater.
  static inline const uint32_t REPEAT_WINDOW_SECS = 12ul * 60 * 60;

  /**
   * \brief  Decide (and record) whether a flood advert from a REPEATER origin
   *         may be repeated by us now: true for the first one, false for any
   *         further advert from the same public key inside the window.
   * \param  origin  the advert's (already signature-verified) identity
   * \param  now     current local time
   */
  static bool allowRepeaterAdvertRepeat(const mesh::Identity& origin, uint32_t now) {
    Entry* entry = findEntry(origin);
    if (entry != nullptr && now >= entry->ts && now - entry->ts < REPEAT_WINDOW_SECS) {
      MESH_DEBUG_PRINTLN("Advert protection: repeater [%02X%02X] advert not repeated (last allowed %us ago)",
                         origin.pub_key[0], origin.pub_key[1], now - entry->ts);
      return false;
    }
    record(origin, now, entry);
    return true;
  }

private:
  // Repeater origins are keyed by a KEY_SIZE-byte prefix of their public key:
  // the key only has to DISTINGUISH origins from each other (the advert's
  // Ed25519 signature was already verified by the caller), so a prefix keeps
  // the table small with negligible collision odds (birthday bound ~K^2/2^64;
  // a collision merely suppresses one advert repeat for at most 12h).
  static inline const uint8_t KEY_SIZE = 8;

  // One record per known repeater origin: when we last allowed an advert from
  // it to be repeated. An entry with ts == 0 is an empty slot; an all-zero
  // key prefix can never match a real Ed25519 identity.
  struct Entry {
    uint8_t key[KEY_SIZE];
    uint32_t ts;
  };

#ifdef MAX_NEIGHBOURS
  // track as many repeaters as the app keeps neighbours (same build flag)
  static inline const int TABLE_SIZE = MAX_NEIGHBOURS;
#else
  static inline const int TABLE_SIZE = 24;   // fallback when no neighbours list is built
#endif
  static inline Entry _table[TABLE_SIZE] = {};   // 50 entries = 600 bytes of RAM

  static Entry* findEntry(const mesh::Identity& origin) {
    for (int i = 0; i < TABLE_SIZE; i++) {
      if (origin.isHashMatch(_table[i].key, KEY_SIZE)) return &_table[i];
    }
    return nullptr;
  }

  static void record(const mesh::Identity& origin, uint32_t now, Entry* entry) {
    if (entry == nullptr) {
      // take the first empty slot, else the least recently recorded one
      // (eviction mirrors MyMesh::putNeighbour)
      uint32_t oldest = 0xFFFFFFFF;
      entry = &_table[0];
      for (int i = 0; i < TABLE_SIZE; i++) {
        if (_table[i].ts == 0) { entry = &_table[i]; break; }
        if (_table[i].ts < oldest) {
          entry = &_table[i];
          oldest = _table[i].ts;
        }
      }
    }
    memcpy(entry->key, origin.pub_key, KEY_SIZE);
    entry->ts = now;
  }
};

#endif // ENABLE_ADVERT_PROTECT
