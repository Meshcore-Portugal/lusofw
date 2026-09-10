#pragma once
// Real SHA-256 (and HMAC-SHA256) for the native lusofw tests. Mirrors the
// rweather/Crypto SHA256.h interface used by src/Utils.cpp and RegionMap.cpp,
// so identity-derived scheduling and transport keys exercise true digests.
// (The stub in test/mocks is only for the other native suites; this shim takes
// precedence via the include order in [env:native_lusofw*].)
#include <cstring>

#include "sha256_impl.h"

class SHA256 {
public:
  SHA256() { shim_sha256::init(_c); }

  void update(const void* data, size_t len) {
    shim_sha256::update(_c, (const uint8_t*)data, len);
  }

  void finalize(uint8_t* out, size_t len) {
    uint8_t full[32];
    shim_sha256::final(_c, full);
    if (len > 32) len = 32;
    memcpy(out, full, len);
  }

  void reset() { shim_sha256::init(_c); }

  void resetHMAC(const uint8_t* key, size_t keyLen) {
    uint8_t k[64];
    memset(k, 0, sizeof k);
    if (keyLen > sizeof k) {
      shim_sha256::Ctx kc;
      shim_sha256::init(kc);
      shim_sha256::update(kc, key, keyLen);
      shim_sha256::final(kc, k);
    } else {
      memcpy(k, key, keyLen);
    }
    for (int i = 0; i < 64; i++) _key[i] = k[i] ^ 0x36;  // ipad block
    shim_sha256::init(_c);
    shim_sha256::update(_c, _key, 64);
    for (int i = 0; i < 64; i++) _key[i] = k[i] ^ 0x5c;  // opad block
  }

  void finalizeHMAC(const uint8_t* key, size_t keyLen, uint8_t* out, size_t len) {
    (void)key;
    (void)keyLen;  // key material was absorbed in resetHMAC
    uint8_t inner[32];
    shim_sha256::final(_c, inner);
    shim_sha256::Ctx oc;
    shim_sha256::init(oc);
    shim_sha256::update(oc, _key, 64);
    shim_sha256::update(oc, inner, 32);
    uint8_t full[32];
    shim_sha256::final(oc, full);
    if (len > 32) len = 32;
    memcpy(out, full, len);
    shim_sha256::init(_c);
  }

private:
  shim_sha256::Ctx _c = {};
  uint8_t _key[64];  // opad block once resetHMAC has run
};
