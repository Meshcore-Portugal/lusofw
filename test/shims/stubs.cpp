// Link stubs for the TransportKeyStore symbols referenced by RegionMap.cpp.
// The audited code paths (auto region assignment, defaults migrations) never
// touch transport keys, so these stay empty. Compiled into every native_lusofw
// test binary via build_src_filter.
#include <helpers/TransportKeyStore.h>

uint16_t TransportKey::calcTransportCode(const mesh::Packet*) const { return 0; }
bool TransportKey::isNull() const {
  for (int i = 0; i < 16; i++) if (key[i]) return false;
  return true;
}
void TransportKeyStore::putCache(uint16_t, const TransportKey&) {}
void TransportKeyStore::getAutoKeyFor(uint16_t, const char*, TransportKey&) {}
int TransportKeyStore::loadKeysFor(uint16_t, TransportKey[], int) { return 0; }
bool TransportKeyStore::saveKeysFor(uint16_t, const TransportKey[], int) { return true; }
bool TransportKeyStore::removeKeys(uint16_t) { return true; }
bool TransportKeyStore::clear() { return true; }
