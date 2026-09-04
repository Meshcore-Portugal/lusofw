#include "Defaults.h"

#include <MeshCore.h>
#include <string.h>

/*
 *                        _
 *   _ __ ___ _ __  _ __ (_) ___
 *  | '__/ _ | '_ \| '_ \| |/ _ \  DUAL ACTION
 *  | | |  __| | | | | | | |  __/  MINT FLAVOUR
 *  |_|  \___|_| |_|_| |_|_|\___|  24 comprimidos para mastigar
 *
 *  Neutralizam de forma rápida e duradoura o excesso de ácido no estômago.
 *
 */

void LusoDefaults::applyDefaults(NodePrefs &prefs, RegionMap &region_map, FILESYSTEM *fs, const char *version) {
  // Always apply the version-independent baseline first.
  prefs.advert_interval = 0;                  // direct adverts are legacy
  prefs.advert_loc_policy = ADVERT_LOC_PREFS; // use coordinates from prefs
  prefs.direct_tx_delay_factor = 0.3f;        // was 0.2
  prefs.flood_advert_interval = 23;           // defaults to 23h on lusofw, when >0 enabled our custom advert handling
  prefs.interference_threshold = 0;           // disable RSSI based listen-before-talk
  prefs.cad_enabled = 1;                      // enable hardware CAD listen-before-talk (set cad off to disable)
  prefs.loop_detect = LOOP_DETECT_MINIMAL;    // default to minimal loop detection
  prefs.path_hash_mode = 1;                   // default to 2 bytes
  prefs.rx_delay_base = 0.0f;                 // turn off by default, was 10.0;
  prefs.tx_delay_factor = 0.5f;               // was 0.25f

  // Then layer any version-specific overrides.
  if (versionLessThan(version, "2026.7.1")) {
    prefs.airtime_factor = 1.0f; // normalize 2026.7.1 to 50%
                                 // future versions will not change it again
                                 // https://meshcore.pt/en/docs/comunicado-lusofw for more info
#if defined(USE_SX1262) || defined(USE_SX1268)
    prefs.rx_boosted_gain = 1;   // config struct changes made this be disabled on edge cases
#endif
  }

  if (versionLessThan(version, "2026.9.1")) {
    // Flag user-customized radio settings so AutoRegions leaves them alone
    prefs.radio_manual = (prefs.airtime_factor != 1.0f || prefs.tx_power_dbm != LORA_TX_POWER) ? 1 : 0;

#if defined(ENABLE_AUTO_REGIONS)
    // Retire the legacy "#portugal" region
    if (fs) {
      region_map.load(fs);
      if (auto legacy = region_map.findByName("#portugal")) {
        if (region_map.removeRegion(*legacy)) {
          region_map.save(fs);
        }
      }
    }
#endif
  }
}

// Platform-consistent file opens, mirroring the vendored openWrite() pattern in
// helpers/RegionMap.cpp (every other copy is a file-local static in read-only
// core files, so this module keeps its own one-per-file pair).
static File openWrite(FILESYSTEM* fs, const char* path) {
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  fs->remove(path); // simpler than truncating
  return fs->open(path, FILE_O_WRITE);
#elif defined(RP2040_PLATFORM)
  return fs->open(path, "w");
#else
  return fs->open(path, "w", true);
#endif
}

static File openRead(FILESYSTEM* fs, const char* path) {
#if defined(RP2040_PLATFORM)
  return fs->open(path, "r");
#elif defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  return fs->open(path, FILE_O_READ);
#else
  return fs->open(path); // ESP32 FS defaults to read mode (no FILE_O_READ there)
#endif
}

void LusoDefaults::readVersion(FILESYSTEM *fs, char *buf, size_t bufLen) {
  if (!buf || bufLen == 0) {
    return;
  }

  buf[0] = 0;
  if (!fs) {
    return;
  }

  File f = openRead(fs, "/lusofw");

  if (!f) {
    return;
  }

  size_t n = f.readBytes(buf, bufLen - 1);
  buf[n] = 0;
  f.close();

  while (n > 0 && (buf[n - 1] == '\r' || buf[n - 1] == '\n' || buf[n - 1] == ' ' || buf[n - 1] == '\t')) {
    buf[--n] = 0;
  }

  MESH_DEBUG_PRINTLN("LusoDefaults: read stored version '%s'", buf);
}

void LusoDefaults::writeVersion(FILESYSTEM *fs, const char *version) {
  if (!fs || !version) {
    return;
  }

  File f = openWrite(fs, "/lusofw");

  if (f) {
    f.print(version);
    f.close();
    MESH_DEBUG_PRINTLN("LusoDefaults: wrote version '%s'", version);
  }
}

bool LusoDefaults::versionLessThan(const char *version, const char *threshold) {
  if (!version || !*version) {
    return true;
  }
  if (version[0] == 'v' || version[0] == 'V') {
    version++;
  }

  for (int i = 0; i < 3; i++) {
    int v = 0;
    while (*version >= '0' && *version <= '9') {
      v = v * 10 + (*version - '0');
      version++;
    }
    int t = 0;
    while (*threshold >= '0' && *threshold <= '9') {
      t = t * 10 + (*threshold - '0');
      threshold++;
    }
    if (v != t) {
      return v < t;
    }
    if (*version == '.') {
      version++;
    }
    if (*threshold == '.') {
      threshold++;
    }
  }
  return false; // equal
}
