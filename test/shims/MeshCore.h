#pragma once
// Host shim for the vendored MeshCore aggregator header: only the constants
// and types the audited sources need. Debug print macros compile out, exactly
// like a production build without MESH_DEBUG.
#include <stdint.h>
#include <stddef.h>
#include <cstdio>
#include <cstring>

#define MAX_HASH_SIZE        8
#define PUB_KEY_SIZE        32
#define PRV_KEY_SIZE        64
#define SEED_SIZE           32
#define SIGNATURE_SIZE      64
#define MAX_ADVERT_DATA_SIZE  32
#define CIPHER_KEY_SIZE     16
#define CIPHER_BLOCK_SIZE   16
#define CIPHER_MAC_SIZE      2
#define PATH_HASH_SIZE       1
#define MAX_PACKET_PAYLOAD  184
#define MAX_GROUP_DATA_LENGTH  (MAX_PACKET_PAYLOAD - CIPHER_BLOCK_SIZE - 3)
#define MAX_PATH_SIZE        64
#define MAX_TRANS_UNIT      255

#define MESH_DEBUG_PRINT(...) do {} while (0)
#define MESH_DEBUG_PRINTLN(...) do {} while (0)

namespace mesh {

class RTCClock {
public:
  RTCClock() {}
  virtual ~RTCClock() {}
  virtual uint32_t getCurrentTime() = 0;
  virtual void setCurrentTime(uint32_t time) = 0;
  virtual void tick() {}
};

}
