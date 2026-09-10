#pragma once
// Host shim: minimal NodePrefs stand-in. Field names and types mirror the
// real NodePrefs in src/helpers/CommonCLI.h; only members consumed by
// AutoRegions.cpp / Defaults.cpp on host are present.
#include <Arduino.h>
#include <helpers/RegionMap.h>

#define ADVERT_LOC_NONE   0
#define ADVERT_LOC_SHARE  1
#define ADVERT_LOC_PREFS  2

#define LOOP_DETECT_OFF       0
#define LOOP_DETECT_MINIMAL   1
#define LOOP_DETECT_MODERATE  2
#define LOOP_DETECT_STRICT    3

struct NodePrefs {
  float airtime_factor = 0;
  char node_name[32];
  double node_lat = 0, node_lon = 0;
  char password[16];
  float freq = 0;
  int8_t tx_power_dbm = 0;
  uint8_t disable_fwd = 0;
  uint8_t advert_interval = 0;
  uint8_t flood_advert_interval = 0;
  float rx_delay_base = 0;
  float tx_delay_factor = 0;
  char guest_password[16];
  float direct_tx_delay_factor = 0;
  uint8_t interference_threshold = 0;
  float bw = 0;
  uint8_t sf = 0;
  uint8_t cr = 0;
  uint8_t cad_enabled = 0;
  uint8_t loop_detect = 0;
  uint8_t path_hash_mode = 0;
  uint8_t rx_boosted_gain = 0;
  uint8_t advert_loc_policy = 0;
  uint8_t radio_manual = 0;
};
