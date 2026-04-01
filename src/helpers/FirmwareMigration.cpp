#include "FirmwareMigration.h"

#include <MeshCore.h>
#include <string.h>

static void v0_0_7(NodePrefs& prefs) {
  prefs.airtime_factor = 9.0f;
  prefs.flood_advert_interval = 24; // defaults to 24h on lusofw, when >0 enabled our custom advert handling
}

static void v0_0_6(NodePrefs &prefs) {
  prefs.advert_interval = 0;         // defaults to disabled on lusofw
  prefs.flood_advert_interval = 0;   // defaults to disabled on lusofw
  prefs.interference_threshold = 14; // enable listen before talk
  prefs.advert_loc_policy = ADVERT_LOC_PREFS;
}

static void v0_0_1(NodePrefs &prefs) {
  prefs.rx_delay_base = 0.0f;           // turn off by default, was 10.0;
  prefs.tx_delay_factor = 0.5f;         // was 0.25f
  prefs.direct_tx_delay_factor = 0.3f;  // was 0.2

#if defined(USE_SX1262) || defined(USE_SX1268)
#ifdef SX126X_RX_BOOSTED_GAIN
  prefs.rx_boosted_gain = SX126X_RX_BOOSTED_GAIN;
#else
  prefs.rx_boosted_gain = 1; // enabled by default;
#endif
#endif
}

const FirmwareMigration::VersionEntry FirmwareMigration::s_versions[] = {
  { "v0.0.1", v0_0_1 },
  { "v0.0.6", v0_0_6 },
  { "v0.0.7", v0_0_7 },
};

int FirmwareMigration::findVersionIndex(const char* version) {
  if (version == nullptr || version[0] == 0) {
    return -1;
  }

  for (size_t i = 0; i < sizeof(s_versions) / sizeof(s_versions[0]); i++) {
    if (strcmp(version, s_versions[i].version) == 0) {
      return (int)i;
    }
  }

  return -1;
}

void FirmwareMigration::applyDefaultsByVersion(const char* oldVersion,
                                               const char* newVersion,
                                               NodePrefs& prefs) {
  int new_idx = findVersionIndex(newVersion);
  if (new_idx < 0) {
    MESH_DEBUG_PRINTLN("FirmwareMigration: newVersion '%s' not in table, skipping", newVersion ? newVersion : "");
    return;
  }

  int old_idx = findVersionIndex(oldVersion);
  int start_idx = old_idx >= 0 ? old_idx + 1 : 0;

  if (start_idx > new_idx) {
    MESH_DEBUG_PRINTLN("FirmwareMigration: '%s' -> '%s' already up to date, skipping", oldVersion ? oldVersion : "", newVersion);
    return;
  }

  MESH_DEBUG_PRINTLN("FirmwareMigration: migrating '%s' -> '%s' (%d step(s))",
                     oldVersion ? oldVersion : "", newVersion, new_idx - start_idx + 1);

  for (int i = start_idx; i <= new_idx; i++) {
    MESH_DEBUG_PRINTLN("FirmwareMigration: applying defaults for %s", s_versions[i].version);
    s_versions[i].defaults(prefs);
  }
}

void FirmwareMigration::readVersion(FILESYSTEM* fs, char* buf, size_t bufLen) {
  if (!buf || bufLen == 0) {
    return;
  }

  buf[0] = 0;
  if (!fs) {
    return;
  }

#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  File f = fs->open("/lusofw", FILE_O_READ);
#elif defined(RP2040_PLATFORM)
  File f = fs->open("/lusofw", "r");
#else
  File f = fs->open("/lusofw");
#endif

  if (!f) {
    return;
  }

  size_t n = f.readBytes(buf, bufLen - 1);
  buf[n] = 0;
  f.close();

  while (n > 0 && (buf[n - 1] == '\r' || buf[n - 1] == '\n' || buf[n - 1] == ' ' || buf[n - 1] == '\t')) {
    buf[--n] = 0;
  }

  MESH_DEBUG_PRINTLN("FirmwareMigration: read stored version '%s'", buf);
}

void FirmwareMigration::writeVersion(FILESYSTEM* fs, const char* version) {
  if (!fs || !version) {
    return;
  }

#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  fs->remove("/lusofw");
  File f = fs->open("/lusofw", FILE_O_WRITE);
#elif defined(RP2040_PLATFORM)
  File f = fs->open("/lusofw", "w");
#else
  File f = fs->open("/lusofw", "w", true);
#endif

  if (f) {
    f.print(version);
    f.close();
    MESH_DEBUG_PRINTLN("FirmwareMigration: wrote version '%s'", version);
  }
}
