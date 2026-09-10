#pragma once
// Host shim for Mesh.h: mesh::Identity plus the mesh::Utils declarations.
// Utils is deliberately DECLARED-ONLY here: the definitions come from the real
// src/Utils.cpp compiled into the native test env, whose SHA256 resolves to the
// real implementation in test/shims/SHA256.h (not the stub in test/mocks).
// Identity-derived scheduling (SmartAdverts) therefore exercises true hashes.
#include "MeshCore.h"

namespace mesh {

class Identity {
public:
  uint8_t pub_key[PUB_KEY_SIZE];
  Identity() { memset(pub_key, 0, sizeof(pub_key)); }
  explicit Identity(const uint8_t* src) { memcpy(pub_key, src, PUB_KEY_SIZE); }
};

class Utils {
public:
  static void sha256(uint8_t* hash, size_t hash_len, const uint8_t* msg, int msg_len);
  static void sha256(uint8_t* hash, size_t hash_len, const uint8_t* frag1, int frag1_len,
                     const uint8_t* frag2, int frag2_len);
};

}
