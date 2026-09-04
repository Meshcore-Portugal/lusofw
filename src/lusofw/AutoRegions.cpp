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
        if (strncmp(name, country_name, country_len) == 0 && name[country_len] == '.') {
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
        region_map.removeRegion(*r);
        return true;
    }
    return false;
}

void AutoRegions::checkRegionAutoAssign(RegionMap& region_map, NodePrefs& prefs, SensorManager& sensors, FILESYSTEM* fs) {
    static float last_checked_lat = -999.0f;
    static float last_checked_lon = -999.0f;
    static char last_checked_name[sizeof(prefs.node_name)] = {0};

    static bool force_initial_check = true;
    static float original_airtime_factor = -1.0f;

    float current_lat = 0.0f;
    float current_lon = 0.0f;

    if (prefs.advert_loc_policy == ADVERT_LOC_PREFS) {
        current_lat = prefs.node_lat;
        current_lon = prefs.node_lon;
    } else if (prefs.advert_loc_policy == ADVERT_LOC_SHARE) {
        current_lat = sensors.node_lat;
        current_lon = sensors.node_lon;
    }

    bool name_changed = (strcmp(prefs.node_name, last_checked_name) != 0);
    bool map_changed = false;

    // If the user's manual coordinates (_prefs) are 0.0, they explicitly cleared them.
    // Unlike a physical GPS losing lock, a manual 0.0 is an explicit command to drop location.
    // We discard the last known location and force a fallback evaluation immediately.
    if (prefs.advert_loc_policy == ADVERT_LOC_PREFS && current_lat == 0.0f && current_lon == 0.0f &&
        last_checked_lat != -999.0f && (last_checked_lat != 0.0f || last_checked_lon != 0.0f)) {
        last_checked_lat = 0.0f;
        last_checked_lon = 0.0f;
        force_initial_check = true;
        map_changed = true;
    }

    bool force_check = force_initial_check || (prefs.advert_loc_policy == ADVERT_LOC_NONE && (last_checked_lat != 0.0f || last_checked_lon != 0.0f));

    float eval_lat = current_lat;
    float eval_lon = current_lon;
    if (current_lat == 0.0f && current_lon == 0.0f && last_checked_lat != -999.0f) {
        eval_lat = last_checked_lat;
        eval_lon = last_checked_lon;
    }

    bool has_gps = (eval_lat != 0.0f || eval_lon != 0.0f);

    float diff_lat = eval_lat > last_checked_lat ? eval_lat - last_checked_lat : last_checked_lat - eval_lat;
    float diff_lon = eval_lon > last_checked_lon ? eval_lon - last_checked_lon : last_checked_lon - eval_lon;
    bool gps_changed = (diff_lat > 0.01f || diff_lon > 0.01f);

    if (!force_check && !name_changed && !gps_changed) {
        return; // No movement, no name change -> do nothing
    }

    force_initial_check = false;

    const char* valid_regions[16];
    int num_valid = 0;
    bool country_matched[NUM_ENABLED_COUNTRIES] = {false};
    bool is_in_europe = false;

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

    if (has_gps) {
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
            for (int c = 0; c < NUM_ENABLED_COUNTRIES && !country_matched[c]; c++) {
                const CountryRegions& country = ENABLED_COUNTRIES[c];
                for (int i = 0; i < country.num_fallback_regions; i++) {
                    if (strcmp(country.fallback_regions[i].prefix, prefix) != 0) continue;
                    country_matched[c] = true;
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

    // Country regions and Europe membership, shared by the GPS and fallback paths
    bool any_country_matched = false;
    for (int c = 0; c < NUM_ENABLED_COUNTRIES; c++) {
        if (!country_matched[c]) continue;
        any_country_matched = true;
        if (ENABLED_COUNTRIES[c].in_europe) is_in_europe = true;
        add_valid_region(ENABLED_COUNTRIES[c].name);
    }
    if (has_gps && !any_country_matched) {
        // Outside every compiled-in country: fall back to the coarse Europe polygon
        if (isPointInPolygon(eval_lat, eval_lon, REGION_EUROPE.rings[0].points, REGION_EUROPE.rings[0].count)) {
            is_in_europe = true;
        }
    }
    if (is_in_europe) {
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

    // Enforce European regulations (max 10% duty cycle / min 9.0 airtime factor,
    // plus band-appropriate tx power) whenever the node is physically located
    // within the European geography.
    // We only overwrite the active setting in RAM if the user's setting currently
    // exceeds what the regulations allow.
    //
    // Skipped when the user has manually set tx power or duty cycle (prefs.radio_manual):
    // their explicit choice wins, even if it is above the regulatory floor.
	//
	// NOTE: We DO NOT call savePrefs() here because flash writes block interrupts,
    // which causes a boot crash (WDT/Hard Fault) on nRF52/RAK4631.
    if (!prefs.radio_manual) {
        // tx power: per ERC/Rec 70-03, 500 mW e.r.p. (27 dBm) is only allowed in
        // 869.4-869.65 MHz; the rest of the 868 band is limited to 25 mW e.r.p.
        // (14 dBm) and the 433 band to 10 mW e.r.p. (10 dBm). Outside Europe the
        // board default (LORA_TX_POWER) applies.
		//
        // NOTE: power limits are measured differently inside and outside Europe.
        // Under FCC rules (USA) power is measured at the transmitter output
        // (conducted power), whereas in the EU the e.r.p. is the radio + antenna
        // combination: TX power + antenna gain - system losses (cable/connector),
        // referenced to a half-wave dipole. So in Europe, restricting TX power
        // alone is not enough to be compliant: antenna gain counts towards the
        // limit.
        int8_t tx_power;
        if (in_europe_flag && prefs.freq >= 869.4f && prefs.freq <= 869.65f) {
            tx_power = 22;  // EU 869.4-869.65 high-power sub-band (500 mW e.r.p.)
        } else if (in_europe_flag && prefs.freq >= 863.0f && prefs.freq <= 870.0f) {
            tx_power = 14;  // EU 868 band (25 mW e.r.p.)
        } else if (in_europe_flag && prefs.freq >= 433.05f && prefs.freq <= 434.79f) {
            tx_power = 10;  // EU 433 band (10 mW e.r.p.)
        } else {
            tx_power = LORA_TX_POWER;
        }
        prefs.tx_power_dbm = tx_power;

        if (in_europe_flag) {
            if (prefs.airtime_factor < 9.0f) {
                // Capture the original permissive duty cycle before restricting it
                if (original_airtime_factor < 0.0f) {
                    original_airtime_factor = prefs.airtime_factor;
                }
                prefs.airtime_factor = 9.0f;
            }
        } else {
            // Restore the original permissive duty cycle if we leave Europe
            if (original_airtime_factor >= 0.0f) {
                prefs.airtime_factor = original_airtime_factor;
                original_airtime_factor = -1.0f;
            }
        }
    }

    // Now apply all valid regions
    if (num_valid > 0) {
        map_changed |= inject_hierarchy(region_map, country_matched, is_in_europe);
        for (int i = 0; i < num_valid; i++) {
            map_changed |= apply_dynamic_region(region_map, valid_regions[i], get_parent_for_region(region_map, valid_regions[i]));
            // MESH_DEBUG_PRINTLN("Auto-Region Assign: %s", valid_regions[i]);
        }
    }

    // Update RAM state unconditionally so we don't re-evaluate immediately
    if (force_check || gps_changed || name_changed) {
        if (has_gps) {
            last_checked_lat = eval_lat;
            last_checked_lon = eval_lon;
        }
        StrHelper::strncpy(last_checked_name, prefs.node_name, sizeof(last_checked_name));
    }

    // Only write to flash if the tree actually changed
    if (map_changed) {
        region_map.save(fs); // Persist assigned regions to NVS
    }
}

#endif // ENABLE_AUTO_REGIONS
