#if defined(ENABLE_AUTO_REGIONS)

#include "AutoRegions.h"

#include <Arduino.h>
#include <helpers/TxtDataHelpers.h>

// Enable or disable specific region hierarchy levels (within every country)
#define ENABLE_REGION_DISTRICTS
#define ENABLE_REGION_MACRO

// Country selection — enable any combination. Each enabled country brings its
// three layers (macro regions, districts, no-GPS fallback catalog). Registry
// order below is the fallback prefix priority order: prefixes are only unique
// within a country's catalog, so the first country claiming a prefix wins.
#define ENABLE_COUNTRY_PT
// #define ENABLE_COUNTRY_ES
// #define ENABLE_COUNTRY_BR

#if !defined(ENABLE_COUNTRY_PT) && !defined(ENABLE_COUNTRY_ES) && !defined(ENABLE_COUNTRY_BR)
#error "Enable at least one country: ENABLE_COUNTRY_PT, ENABLE_COUNTRY_ES, ENABLE_COUNTRY_BR"
#endif

#ifdef ENABLE_COUNTRY_PT
  #include "lusofw/regions/pt_regions.h"
#endif
#ifdef ENABLE_COUNTRY_ES
  #include "lusofw/regions/es_regions.h"
#endif
#ifdef ENABLE_COUNTRY_BR
  #include "lusofw/regions/br_regions.h"
#endif

#include "lusofw/regions/eu_regions.h"

// Registry of the compiled-in countries, in fallback priority order.
static const CountryRegions ENABLED_COUNTRIES[] = {
#ifdef ENABLE_COUNTRY_PT
    {PT_REGION_NAME, PT_IN_EUROPE, PT_MACRO_REGIONS, NUM_PT_MACRO_REGIONS, PT_DISTRICTS, NUM_PT_DISTRICTS, PT_FALLBACK_REGIONS, NUM_PT_FALLBACK_REGIONS},
#endif
#ifdef ENABLE_COUNTRY_ES
    {ES_REGION_NAME, ES_IN_EUROPE, ES_MACRO_REGIONS, NUM_ES_MACRO_REGIONS, ES_DISTRICTS, NUM_ES_DISTRICTS, ES_FALLBACK_REGIONS, NUM_ES_FALLBACK_REGIONS},
#endif
#ifdef ENABLE_COUNTRY_BR
    {BR_REGION_NAME, BR_IN_EUROPE, BR_MACRO_REGIONS, NUM_BR_MACRO_REGIONS, BR_DISTRICTS, NUM_BR_DISTRICTS, BR_FALLBACK_REGIONS, NUM_BR_FALLBACK_REGIONS},
#endif
};
static const int NUM_ENABLED_COUNTRIES = sizeof(ENABLED_COUNTRIES) / sizeof(ENABLED_COUNTRIES[0]);

bool AutoRegions::in_europe_flag = false;

bool AutoRegions::isNodeInEurope() {
    return in_europe_flag;
}
// Classic Ray-Casting algorithm (Fast and allocation-free)
static bool isPointInPolygon(float lat, float lon, const GeoPoint* poly, int num_points) {
    bool inside = false;
    for (int i = 0, j = num_points - 1; i < num_points; j = i++) {
        // Check if the horizontal ray intersects the segment between nodes i and j
        if (((poly[i].lon > lon) != (poly[j].lon > lon)) &&
            (lat < (poly[j].lat - poly[i].lat) * (lon - poly[i].lon) / (poly[j].lon - poly[i].lon) + poly[i].lat)) {
            inside = !inside;
        }
    }
    return inside;
}

// Which hierarchy levels are compiled in; fallback entries are filtered by this mask.
static uint8_t enabledRegionKinds() {
    uint8_t k = 0;
#ifdef ENABLE_REGION_MACRO
    k |= KIND_MACRO;
#endif
#ifdef ENABLE_REGION_DISTRICTS
    k |= KIND_DISTRICT;
#endif
    return k;
}

bool AutoRegions::inject_hierarchy(RegionMap& region_map, const bool* country_matched, bool create_eu) {
    bool changed = false;
    if (create_eu) {
        auto r = region_map.findByName("#europe");
        if (!r) {
            r = region_map.putRegion("#europe", 0);
            if (r) {
                r->flags |= REGION_AUTO_ASSIGN;
                changed = true;
            }
        }
    }

    for (int c = 0; c < NUM_ENABLED_COUNTRIES; c++) {
        if (!country_matched[c]) continue;
        const char* name = ENABLED_COUNTRIES[c].name;
        auto r = region_map.findByName(name);
        if (!r) {
            r = region_map.putRegion(name, get_parent_for_region(region_map, name));
            if (r) {
                r->flags |= REGION_AUTO_ASSIGN;
                changed = true;
            }
        } else {
            uint16_t expected_parent = get_parent_for_region(region_map, name);
            if (r->parent != expected_parent) {
                r->parent = expected_parent;
                changed = true;
            }
        }
    }
    return changed;
}

uint16_t AutoRegions::get_parent_for_region(RegionMap& region_map, const char* name) {
    for (int c = 0; c < NUM_ENABLED_COUNTRIES; c++) {
        const char* country_name = ENABLED_COUNTRIES[c].name;
        size_t country_len = strlen(country_name);
        if (strcmp(name, country_name) == 0) {
            auto p = region_map.findByName("#europe");
            return p ? p->id : 0;
        }
        if (strncmp(name, country_name, country_len) == 0 && name[country_len] == '-') {
            auto p = region_map.findByName(country_name);
            return p ? p->id : 0;
        }
    }
    return 0;
}

bool AutoRegions::enable_region_path(RegionMap& region_map, const char* name) {
    bool changed = false;
    auto r = region_map.findByName(name);
    while (r) {
        if (r->flags & REGION_DENY_FLOOD) {
            r->flags &= ~REGION_DENY_FLOOD; // Enable flood for this region and all its parents
            changed = true;
        }
        if (r->parent != region_map.getWildcard().id) {
            r = region_map.findById(r->parent);
        } else {
            break;
        }
    }
    return changed;
}

bool AutoRegions::apply_dynamic_region(RegionMap& region_map, const char* reg_name, uint16_t parent_id) {
    bool changed = false;
    auto dynamic_region = region_map.findByName(reg_name);
    if (!dynamic_region) {
        dynamic_region = region_map.putRegion(reg_name, parent_id);
        if (dynamic_region) {
            dynamic_region->flags |= REGION_AUTO_ASSIGN;
            changed = true;
        }
    } else if (dynamic_region->parent != parent_id) {
        dynamic_region->parent = parent_id;
        changed = true;
    }
    changed |= enable_region_path(region_map, reg_name);
    return changed;
}

bool AutoRegions::remove_outdated_region(RegionMap& region_map, const char* reg_name) {
    auto r = region_map.findByName(reg_name);
    if (r && (r->flags & REGION_AUTO_ASSIGN)) {
        return region_map.removeRegion(*r); // false when children still reference it
    }
    return false;
}

// Tx power and duty cycle are derived state: the region sets both, and they
// are re-derived on every evaluation and every frequency change until the
// user sets either one manually (prefs.radio_manual, latched by the CLI
// set tx / dutycycle / af handlers in CommonCLI). Limits are applied to the
// conducted/configured value (FCC-style measurement): antenna gain is
// unknowable, so the EU e.r.p. numbers are used directly; the variant
// default (LORA_TX_POWER) is never exceeded. Only mutates prefs — callers
// apply tx power to the radio themselves. Does NOT savePrefs (flash writes
// block interrupts -> WDT/hard fault boot on nRF52/RAK4631).
void AutoRegions::applyRadioRegulation(NodePrefs& prefs, float freq) {
    if (prefs.radio_manual) return;

    // Per ERC/Rec 70-03: 500 mW e.r.p. (27 dBm) only in 869.4-869.65 MHz;
    // the rest of the 868 band is 25 mW e.r.p. (14 dBm); the 433 band is
    // 10 mW e.r.p. (10 dBm). On an unrecognized frequency in Europe the most
    // restrictive limit applies. Outside Europe the board default governs.
    int8_t tx_power;
    if (!in_europe_flag) {
        tx_power = LORA_TX_POWER;
    } else if (freq >= 869.4f && freq <= 869.65f) {
        tx_power = 22;
    } else if (freq >= 863.0f && freq <= 870.0f) {
        tx_power = 14;
    } else if (freq >= 433.05f && freq <= 434.79f) {
        tx_power = 10;
    } else {
        tx_power = 10; // in Europe on a frequency with no EU allocation
    }
    if (tx_power > LORA_TX_POWER) tx_power = LORA_TX_POWER;

    prefs.tx_power_dbm = tx_power;
    prefs.airtime_factor = in_europe_flag ? 9.0f : 1.0f;
}

void AutoRegions::checkRegionAutoAssign(RegionMap& region_map, NodePrefs& prefs, FILESYSTEM* fs) {
    static float last_checked_lat = -999.0f;
    static float last_checked_lon = -999.0f;
    static char last_checked_name[sizeof(prefs.node_name)] = {0};
    static uint8_t last_loc_policy = 0xFF;

    static bool force_initial_check = true;

    // Hardware GPS is deliberately not consulted — LusoFW disables GPS on all
    // repeaters (stationary infrastructure; see the policy note in AutoRegions.h).
    // Region assignment uses only the node's configured coordinates.
    // Evaluation cascade, first match wins:
    //   1. node coordinates non-zero and location policy not NONE -> polygons
    //   2. otherwise the node-name prefix fallback
    //   3. nothing matched -> the compile-time default coordinates, for
    //      regulation only — no regions are created
    bool loc_available = (prefs.node_lat != 0.0f || prefs.node_lon != 0.0f) &&
                         prefs.advert_loc_policy != ADVERT_LOC_NONE;
    float eval_lat = prefs.node_lat;
    float eval_lon = prefs.node_lon;

    bool name_changed = (strcmp(prefs.node_name, last_checked_name) != 0);
    bool policy_changed = (prefs.advert_loc_policy != last_loc_policy);
    bool map_changed = false;

    float diff_lat = eval_lat > last_checked_lat ? eval_lat - last_checked_lat : last_checked_lat - eval_lat;
    float diff_lon = eval_lon > last_checked_lon ? eval_lon - last_checked_lon : last_checked_lon - eval_lon;
    bool coords_changed = (diff_lat > 0.01f || diff_lon > 0.01f);

    if (!force_initial_check && !name_changed && !policy_changed && !coords_changed) {
        return; // No coordinate, policy or name change -> do nothing
    }

    force_initial_check = false;

    const char* valid_regions[16];
    int num_valid = 0;
    bool country_matched[NUM_ENABLED_COUNTRIES] = {false};
    bool is_in_europe = false;

    // Overflow note: if more than 16 regions match, extras are silently dropped
    // here — and since polygon names are added before the country names and
    // "#europe" appended by the shared block below, overflow would drop exactly
    // those structural entries while country_matched[]/is_in_europe stay true.
    // The removal sweep would then delete them and inject_hierarchy recreate
    // them on every evaluation (repeated /regions2 flash rewrites). Reaching it
    // is possible but requires very specific conditions: a node inside more
    // than ~14 matching polygons, which needs multiple countries' geometries
    // to overlap (or malformed data) — districts and macro regions each
    // partition their country, so well-formed data matches ~1 macro + ~1
    // district per country (~4 entries with the country and "#europe").
    auto add_valid_region = [&](const char* name) {
        if (num_valid < 16) {
            valid_regions[num_valid++] = name;
        }
    };

    auto is_region_valid = [&](const char* name) {
        for (int i = 0; i < num_valid; i++) {
            if (strcmp(valid_regions[i], name) == 0) return true;
        }
        return false;
    };

    // Coordinate evaluation needs compiled-in polygon data; with both hierarchy
    // levels disabled there is none, so fall back to the name-prefix path —
    // nodes of the same name then get the same country assignment either way.
    bool has_polygon_data = false;
    for (int c = 0; c < NUM_ENABLED_COUNTRIES && !has_polygon_data; c++) {
        has_polygon_data = ENABLED_COUNTRIES[c].num_macro_regions > 0 || ENABLED_COUNTRIES[c].num_districts > 0;
    }

    if (loc_available && has_polygon_data) {
        auto evaluate_polygon_array = [&](int country_idx, const RegionPolygon* polys, int count) {
            for (int i = 0; i < count; i++) {
                for (int j = 0; j < polys[i].ring_count; j++) {
                    if (isPointInPolygon(eval_lat, eval_lon, polys[i].rings[j].points, polys[i].rings[j].count)) {
                        country_matched[country_idx] = true;
                        add_valid_region(polys[i].name);
                        break; // region matched; no need to test its remaining rings
                    }
                }
            }
        };

        for (int c = 0; c < NUM_ENABLED_COUNTRIES; c++) {
            evaluate_polygon_array(c, ENABLED_COUNTRIES[c].macro_regions, ENABLED_COUNTRIES[c].num_macro_regions);
            evaluate_polygon_array(c, ENABLED_COUNTRIES[c].districts, ENABLED_COUNTRIES[c].num_districts);
        }
    } else {
        if (prefs.node_name[0] != '\0' && prefs.node_name[1] != '\0' && prefs.node_name[2] == '.') {
            char prefix[3];
            prefix[0] = toupper(prefs.node_name[0]);
            prefix[1] = toupper(prefs.node_name[1]);
            prefix[2] = '\0';

            const uint8_t enabled_kinds = enabledRegionKinds();

            // First country claiming the prefix wins (registry order).
            bool prefix_claimed = false;
            for (int c = 0; c < NUM_ENABLED_COUNTRIES && !prefix_claimed; c++) {
                const CountryRegions& country = ENABLED_COUNTRIES[c];
                for (int i = 0; i < country.num_fallback_regions; i++) {
                    if (strcmp(country.fallback_regions[i].prefix, prefix) != 0) continue;
                    country_matched[c] = true;
                    prefix_claimed = true;
                    is_in_europe = country.in_europe;
                    for (int j = 0; j < country.fallback_regions[i].num_regions; j++) {
                        const FallbackRegion& fr = country.fallback_regions[i].regions[j];
                        if (!(fr.kinds & enabled_kinds)) continue; // level not compiled in
                        add_valid_region(fr.name);
                    }
                    break;
                }
            }
        }
    }

    // Country regions and Europe membership, shared by the coordinate and fallback paths
    bool any_country_matched = false;
    for (int c = 0; c < NUM_ENABLED_COUNTRIES; c++) {
        if (!country_matched[c]) continue;
        any_country_matched = true;
        if (ENABLED_COUNTRIES[c].in_europe) is_in_europe = true;
        add_valid_region(ENABLED_COUNTRIES[c].name);
    }
    bool from_defaults = false;
    if (!any_country_matched) {
        if (loc_available) {
            // Outside every compiled-in country: fall back to the coarse Europe polygon
            if (isPointInPolygon(eval_lat, eval_lon, REGION_EUROPE.rings[0].points, REGION_EUROPE.rings[0].count)) {
                is_in_europe = true;
            }
        } else {
            // Nothing matched: regulation follows the compile-time defaults only
            from_defaults = true;
            if (isPointInPolygon(ADVERT_LAT, ADVERT_LON, REGION_EUROPE.rings[0].points, REGION_EUROPE.rings[0].count)) {
                is_in_europe = true;
            }
        }
    }
    if (is_in_europe && !from_defaults) {
        add_valid_region("#europe");
    }

    // Now remove any region that is NOT in valid_regions
    for (int c = 0; c < NUM_ENABLED_COUNTRIES; c++) {
        const CountryRegions& country = ENABLED_COUNTRIES[c];
        for (int i = 0; i < country.num_macro_regions; i++) {
            if (!is_region_valid(country.macro_regions[i].name)) map_changed |= remove_outdated_region(region_map, country.macro_regions[i].name);
        }
        for (int i = 0; i < country.num_districts; i++) {
            if (!is_region_valid(country.districts[i].name)) map_changed |= remove_outdated_region(region_map, country.districts[i].name);
        }
        for (int i = 0; i < country.num_fallback_regions; i++) {
            for (int j = 0; j < country.fallback_regions[i].num_regions; j++) {
                if (!is_region_valid(country.fallback_regions[i].regions[j].name)) {
                    map_changed |= remove_outdated_region(region_map, country.fallback_regions[i].regions[j].name);
                }
            }
        }
        if (!is_region_valid(country.name)) map_changed |= remove_outdated_region(region_map, country.name);
    }
    if (!is_region_valid("#europe")) map_changed |= remove_outdated_region(region_map, "#europe");

    in_europe_flag = is_in_europe;

    // Tx power / duty cycle follow the region until the user sets them manually
    applyRadioRegulation(prefs, prefs.freq);

    // Now apply all valid regions
    if (num_valid > 0) {
        map_changed |= inject_hierarchy(region_map, country_matched, is_in_europe && !from_defaults);
        for (int i = 0; i < num_valid; i++) {
            map_changed |= apply_dynamic_region(region_map, valid_regions[i], get_parent_for_region(region_map, valid_regions[i]));
            // MESH_DEBUG_PRINTLN("Auto-Region Assign: %s", valid_regions[i]);
        }
    }

    // Update RAM state so we don't re-evaluate immediately
    last_checked_lat = prefs.node_lat;
    last_checked_lon = prefs.node_lon;
    StrHelper::strncpy(last_checked_name, prefs.node_name, sizeof(last_checked_name));
    last_loc_policy = prefs.advert_loc_policy;

    // Only write to flash if the tree actually changed
    if (map_changed) {
        region_map.save(fs); // Persist assigned regions to NVS
    }
}

#endif // ENABLE_AUTO_REGIONS
