#pragma once

#include <helpers/CommonCLI.h>

// Region geometry primitives. Consumed by the region-data headers in regions/.
struct GeoPoint {
  float lat;
  float lon;
};

// One closed ring of a region polygon (last vertex == first vertex).
struct RegionRing {
  const GeoPoint* points;
  uint16_t count;
};

// A region is one or more rings (multi-part regions carry one ring per island).
struct RegionPolygon {
  const char* name;
  const RegionRing* rings;
  uint8_t ring_count;
};

// One no-GPS fallback assignment: a region name.
struct FallbackRegion {
  const char* name;
};

// Node-name prefix (e.g. "AV") -> regions a node there belongs to.
struct RegionFallback {
  const char* prefix;
  const FallbackRegion* regions;
  int num_regions;
};

// One compiled-in country: identity plus its three region layers. Countries
// are registered in AutoRegions.cpp (ENABLED_COUNTRIES); registry order is the
// fallback prefix priority order.
struct CountryRegions {
  const char* name; // "#pt"
  bool in_europe;   // applies EU duty-cycle / tx-power enforcement
  const RegionPolygon* macro_regions;
  int num_macro_regions;
  const RegionPolygon* districts;
  int num_districts;
  const RegionFallback* fallback_regions;
  int num_fallback_regions;
};

// Self-pairs a country's tables with their own counts, so a registry row can
// never mix one table with another's count (a hand-written 8-field row could,
// and a swapped pair reads past the shorter array with no runtime check).
#define DECLARE_COUNTRY(cc) \
    {cc##_REGION_NAME, cc##_IN_EUROPE, \
     cc##_MACRO_REGIONS, NUM_##cc##_MACRO_REGIONS, \
     cc##_DISTRICTS, NUM_##cc##_DISTRICTS, \
     cc##_FALLBACK_REGIONS, NUM_##cc##_FALLBACK_REGIONS}

// Automatic geographical region assignment.
//
// Hardware GPS is deliberately not consulted: LusoFW disables GPS support on
// all repeaters, as repeaters are stationary pieces of infrastructure. If
// that policy ever changes, AutoRegions must be adapted to use coordinates
// provided by GPS hardware. Until then, evaluation uses the node's configured
// coordinates, then the node-name prefix fallback, then the compile-time
// default coordinates (regulation only). This module is intentionally
// decoupled from any specific application class: callers pass the RegionMap,
// NodePrefs and filesystem it should operate on. It depends only on MeshCore
// library types, so it can be compiled into any firmware environment without
// pulling in an app-specific header.
class AutoRegions {
public:
  static void checkRegionAutoAssign(RegionMap& region_map, NodePrefs& prefs, FILESYSTEM* fs);
  static bool isNodeInEurope();
  // Tx power / duty cycle are derived state: re-derive both for the given
  // frequency (call wherever frequency or location changes). Only mutates
  // prefs — the caller applies tx power to the radio. No-ops once the user
  // has set either value manually (prefs.radio_manual, latched by the CLI
  // set tx / dutycycle / af handlers in CommonCLI).
  static void applyRadioRegulation(NodePrefs& prefs, float freq);

private:
  static bool in_europe_flag;
  static bool inject_hierarchy(RegionMap& region_map, const bool* country_matched, bool create_eu);
  static uint16_t get_parent_for_region(RegionMap& region_map, const char* name);
  static bool enable_region_path(RegionMap& region_map, const char* name);
  static bool apply_dynamic_region(RegionMap& region_map, const char* reg_name, uint16_t parent_id);
};
