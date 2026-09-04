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

// Region hierarchy levels. A name normally belongs to one level; the tags let
// the fallback catalog state which level each entry refers to.
enum RegionKind : uint8_t {
  KIND_MACRO    = 1 << 0,
  KIND_DISTRICT = 1 << 1,
};

// One no-GPS fallback assignment: region name plus the level it belongs to.
struct FallbackRegion {
  const char* name;
  uint8_t kinds;
};

// Node-name prefix (e.g. "AV") -> regions a node there belongs to.
struct RegionFallback {
  const char* prefix;
  const FallbackRegion* regions;
  int num_regions;
};

// One compiled-in country: identity plus its three region layers. Disabled
// hierarchy levels appear as null tables with a zero count. Countries are
// registered in AutoRegions.cpp (ENABLED_COUNTRIES); registry order is the
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

// Automatic geographical region assignment.
//
// This module is intentionally decoupled from any specific application class:
// callers pass the RegionMap, NodePrefs, SensorManager and filesystem it should
// operate on. It depends only on MeshCore library types, so it can be compiled
// into any firmware environment without pulling in an app-specific header.
class AutoRegions {
public:
  static void checkRegionAutoAssign(RegionMap& region_map, NodePrefs& prefs, SensorManager& sensors, FILESYSTEM* fs);
  static bool isNodeInEurope();

private:
  static bool in_europe_flag;
  static bool inject_hierarchy(RegionMap& region_map, const bool* country_matched, bool create_eu);
  static uint16_t get_parent_for_region(RegionMap& region_map, const char* name);
  static bool enable_region_path(RegionMap& region_map, const char* name);
  static bool apply_dynamic_region(RegionMap& region_map, const char* reg_name, uint16_t parent_id);
  static bool remove_outdated_region(RegionMap& region_map, const char* reg_name);
};
