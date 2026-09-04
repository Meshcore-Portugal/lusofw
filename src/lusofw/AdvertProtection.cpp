#if defined(ENABLE_ADVERT_PROTECT)

#include "AdvertProtection.h"

#include <Mesh.h>
#include <string.h>

// Repeater origins are keyed by a KEY_SIZE-byte prefix of their public key:
// the key only has to DISTINGUISH origins from each other (the advert's
// Ed25519 signature was already verified by the caller), so a prefix keeps
// the table small with negligible collision odds (birthday bound ~K^2/2^64;
// a collision merely suppresses one advert repeat for at most 12h).
static const uint8_t KEY_SIZE = 8;

// One record per known repeater origin: when we last allowed an advert from
// it to be repeated. An entry with ts == 0 is an empty slot; an all-zero
// key prefix can never match a real Ed25519 identity.
struct Entry {
  uint8_t key[KEY_SIZE];
  uint32_t ts;
};

#ifdef MAX_NEIGHBOURS
// track as many repeaters as the app keeps neighbours (same build flag)
static const int TABLE_SIZE = MAX_NEIGHBOURS;
#else
static const int TABLE_SIZE = 24;   // fallback when no neighbours list is built
#endif
static Entry table[TABLE_SIZE] = {};   // 50 entries = 600 bytes of RAM

static Entry* findEntry(const mesh::Identity& origin) {
  for (int i = 0; i < TABLE_SIZE; i++) {
    if (origin.isHashMatch(table[i].key, KEY_SIZE)) return &table[i];
  }
  return nullptr;
}

static void record(const mesh::Identity& origin, uint32_t now, Entry* entry) {
  if (entry == nullptr) {
    // take the first empty slot, else the least recently recorded one
    // (eviction mirrors MyMesh::putNeighbour)
    uint32_t oldest = 0xFFFFFFFF;
    entry = &table[0];
    for (int i = 0; i < TABLE_SIZE; i++) {
      if (table[i].ts == 0) { entry = &table[i]; break; }
      if (table[i].ts < oldest) {
        entry = &table[i];
        oldest = table[i].ts;
      }
    }
  }
  memcpy(entry->key, origin.pub_key, KEY_SIZE);
  entry->ts = now;
}

bool AdvertProtection::allowRepeaterAdvertRepeat(const mesh::Identity& origin, uint32_t now) {
  Entry* entry = findEntry(origin);
  if (entry != nullptr && now >= entry->ts && now - entry->ts < REPEAT_WINDOW_SECS) {
    MESH_DEBUG_PRINTLN("Advert protection: repeater [%02X%02X] advert not repeated (last allowed %us ago)",
                       origin.pub_key[0], origin.pub_key[1], now - entry->ts);
    return false;
  }
  record(origin, now, entry);
  return true;
}

#endif // ENABLE_ADVERT_PROTECT
