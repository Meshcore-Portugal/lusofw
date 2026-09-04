#if defined(ENABLE_AUTO_REGIONS)

#include "AutoRegions.h"

#include <Arduino.h>
#include <helpers/TxtDataHelpers.h>

// Define the distance buffer for each region type (0, 5, 10, 20, 35, 50)
#define REGION_DISTRICTS_BUFFER 0 // km
#define REGION_NUTS2_BUFFER     0 // km
#define REGION_CIMS_BUFFER      0 // km

// Enable or disable specific region types
#define ENABLE_REGION_DISTRICTS
#define ENABLE_REGION_NUTS2
// #define ENABLE_REGION_CIMS
// #define ENABLE_REGION_IATA

#define STR_HELPER(x)               #x
#define STR(x)                      STR_HELPER(x)

#define HEADER_DISTRICTS_FILE(b)    lusofw/regions/pt_districts_##b##km.h
#define HEADER_NUTS2_FILE(b)        lusofw/regions/pt_anepc_nuts2_##b##km.h
#define HEADER_CIMS_FILE(b)         lusofw/regions/pt_anepc_cims_##b##km.h

#define HEADER_DISTRICTS(b)         STR(HEADER_DISTRICTS_FILE(b))
#define HEADER_NUTS2(b)             STR(HEADER_NUTS2_FILE(b))
#define HEADER_CIMS(b)              STR(HEADER_CIMS_FILE(b))

#ifdef ENABLE_REGION_DISTRICTS
  #include HEADER_DISTRICTS(REGION_DISTRICTS_BUFFER)
#endif

#ifdef ENABLE_REGION_NUTS2
  #include HEADER_NUTS2(REGION_NUTS2_BUFFER)
#endif

#ifdef ENABLE_REGION_CIMS
  #include HEADER_CIMS(REGION_CIMS_BUFFER)
#endif

#ifdef ENABLE_REGION_IATA
  #include "lusofw/regions/pt_iata.h"
#endif

#include "lusofw/regions/pt_regions_no_gps_fallback.h"
#include "lusofw/regions/eu_region_0km.h"

#ifndef LORA_TX_POWER
#define LORA_TX_POWER 22
#endif

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

bool AutoRegions::inject_hierarchy(RegionMap& region_map, bool create_eu, bool create_pt) {
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

    if (create_pt) {
        auto r = region_map.findByName("#pt");
        if (!r) {
            r = region_map.putRegion("#pt", get_parent_for_region(region_map, "#pt"));
            if (r) {
                r->flags |= REGION_AUTO_ASSIGN;
                changed = true;
            }
        } else {
            uint16_t expected_parent = get_parent_for_region(region_map, "#pt");
            if (r->parent != expected_parent) {
                r->parent = expected_parent;
                changed = true;
            }
        }
    }
    return changed;
}

uint16_t AutoRegions::get_parent_for_region(RegionMap& region_map, const char* name) {
    if (strcmp(name, "#pt") == 0) {
        auto p = region_map.findByName("#europe");
        return p ? p->id : 0;
    }
    if (strncmp(name, "#pt.", 4) == 0) {
        auto p = region_map.findByName("#pt");
        return p ? p->id : 0;
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
    bool is_in_portugal = false;
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
        auto evaluate_polygon_array = [&](const RegionPolygon* polys, int count) {
            for (int i = 0; i < count; i++) {
                if (isPointInPolygon(eval_lat, eval_lon, polys[i].points, polys[i].num_points)) {
                    is_in_portugal = true;
                    add_valid_region(polys[i].name);
                }
            }
        };

        #ifdef ENABLE_REGION_DISTRICTS
        evaluate_polygon_array(PORTUGAL_DISTRICTS, NUM_PORTUGAL_DISTRICTS);
        #endif

        #ifdef ENABLE_REGION_NUTS2
        evaluate_polygon_array(PORTUGAL_ANEPC_NUTS2, NUM_PORTUGAL_ANEPC_NUTS2);
        #endif

        #ifdef ENABLE_REGION_CIMS
        evaluate_polygon_array(PORTUGAL_ANEPC_CIMS, NUM_PORTUGAL_ANEPC_CIMS);
        #endif

        #ifdef ENABLE_REGION_IATA
        if (is_in_portugal) {
            const char* closest_iata = nullptr;
            float min_dist_sq = 999999.0f;

            for (int i = 0; i < 7; i++) {
                float dx = eval_lat - iata_hubs[i].lat;
                float dy = (eval_lon - iata_hubs[i].lon) * 0.76f;
                float dist_sq = dx * dx + dy * dy;
                if (dist_sq < min_dist_sq) {
                    min_dist_sq = dist_sq;
                    closest_iata = iata_hubs[i].name;
                }
            }
            if (closest_iata) {
                add_valid_region(closest_iata);
            }
        }
        #endif

        if (is_in_portugal) {
            is_in_europe = true;
            add_valid_region("#pt");
        } else {
            if (isPointInPolygon(eval_lat, eval_lon, REGION_EUROPE.points, REGION_EUROPE.num_points)) {
                is_in_europe = true;
            }
        }

        if (is_in_europe) {
            add_valid_region("#europe");
        }
    } else {
        if (prefs.node_name[0] != '\0' && prefs.node_name[1] != '\0' && prefs.node_name[2] == '.') {
            char prefix[3];
            prefix[0] = toupper(prefs.node_name[0]);
            prefix[1] = toupper(prefs.node_name[1]);
            prefix[2] = '\0';

            for (int i = 0; i < NUM_FALLBACK_REGIONS; i++) {
                if (strcmp(FALLBACK_REGIONS[i].prefix, prefix) == 0) {
                    is_in_portugal = true;
                    is_in_europe = true;
                    add_valid_region("#pt");
                    add_valid_region("#europe");

                    for (int j = 0; j < FALLBACK_REGIONS[i].num_regions; j++) {
                        const char* reg_name = FALLBACK_REGIONS[i].regions[j];
#ifndef ENABLE_REGION_IATA
                        if (strstr(reg_name, "iata") != nullptr) continue;
#endif

                        bool is_nuts2 = false;
                        const char* nuts2_list[] = {
                            "#pt-alentejo", "#pt-algarve", "#pt-centro", "#pt-lisboa-vale-do-tejo",
                            "#pt-norte", "#pt-madeira", "#pt-acores"
                        };
                        for (int k = 0; k < 7; k++) {
                            if (strcmp(reg_name, nuts2_list[k]) == 0) { is_nuts2 = true; break; }
                        }

                        bool is_cim = false;
                        const char* cims_list[] = {
                            "#pt-alto-minho", "#pt-cavado", "#pt-ave", "#pt-porto", "#pt-alto-tamega-e-barroso",
                            "#pt-tamega-e-sousa", "#pt-douro", "#pt-terras-tras-os-montes", "#pt-algarve",
                            "#pt-regiao-de-aveiro", "#pt-regiao-de-coimbra", "#pt-regiao-de-leiria",
                            "#pt-viseu-dao-lafoes", "#pt-beira-baixa", "#pt-beiras-e-serra-estrela",
                            "#pt-grande-lisboa", "#pt-peninsula-de-setubal", "#pt-alentejo-litoral",
                            "#pt-baixo-alentejo", "#pt-alto-alentejo", "#pt-alentejo-central", "#pt-oeste",
                            "#pt-medio-tejo", "#pt-leziria-do-tejo", "#pt-acores"
                        };
                        for (int k = 0; k < 25; k++) {
                            if (strcmp(reg_name, cims_list[k]) == 0) { is_cim = true; break; }
                        }

#ifndef ENABLE_REGION_NUTS2
                        if (is_nuts2 && !is_cim) continue;
#endif

#ifndef ENABLE_REGION_CIMS
                        if (is_cim && !is_nuts2) continue;
#endif

#if !defined(ENABLE_REGION_NUTS2) && !defined(ENABLE_REGION_CIMS)
                        if (is_nuts2 || is_cim) continue;
#endif
                        add_valid_region(reg_name);
                    }
                    break;
                }
            }
        }
    }

    // Now remove any region that is NOT in valid_regions
    #ifdef ENABLE_REGION_DISTRICTS
    for (int i = 0; i < NUM_PORTUGAL_DISTRICTS; i++) {
        if (!is_region_valid(PORTUGAL_DISTRICTS[i].name)) map_changed |= remove_outdated_region(region_map, PORTUGAL_DISTRICTS[i].name);
    }
    #endif

    #ifdef ENABLE_REGION_NUTS2
    for (int i = 0; i < NUM_PORTUGAL_ANEPC_NUTS2; i++) {
        if (!is_region_valid(PORTUGAL_ANEPC_NUTS2[i].name)) map_changed |= remove_outdated_region(region_map, PORTUGAL_ANEPC_NUTS2[i].name);
    }
    #endif

    #ifdef ENABLE_REGION_CIMS
    for (int i = 0; i < NUM_PORTUGAL_ANEPC_CIMS; i++) {
        if (!is_region_valid(PORTUGAL_ANEPC_CIMS[i].name)) map_changed |= remove_outdated_region(region_map, PORTUGAL_ANEPC_CIMS[i].name);
    }
    #endif

    #ifdef ENABLE_REGION_IATA
    for (int i = 0; i < 7; i++) {
        if (!is_region_valid(iata_hubs[i].name)) map_changed |= remove_outdated_region(region_map, iata_hubs[i].name);
    }
    #endif

    for (int i = 0; i < NUM_FALLBACK_REGIONS; i++) {
        for (int j = 0; j < FALLBACK_REGIONS[i].num_regions; j++) {
            if (!is_region_valid(FALLBACK_REGIONS[i].regions[j])) {
                map_changed |= remove_outdated_region(region_map, FALLBACK_REGIONS[i].regions[j]);
            }
        }
    }

    if (!is_region_valid("#pt")) map_changed |= remove_outdated_region(region_map, "#pt");
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
        map_changed |= inject_hierarchy(region_map, is_in_europe, is_in_portugal);
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
