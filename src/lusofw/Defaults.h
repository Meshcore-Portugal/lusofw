#pragma once

#include <helpers/CommonCLI.h>

// Applies and tracks lusofw-specific NodePrefs defaults.
//
// On every firmware version change (including the very first boot),
// applyDefaults() overwrites the lusofw-managed preference fields, then the
// caller persists them and stamps the new version. User-defined values
// for these fields are intentionally reset on version change.
class LusoDefaults {
 public:
  // Applies baseline defaults, then any version-specific overrides for the
  // given firmware version string (e.g. "2026.7.2-rc2"). Also applies
  // version-specific one-shot migrations to the persisted region map — the
  // map is loaded and saved back here, only when a migration actually fires
  // and only if something was migrated.
  // Returns false when a region-map migration is blocked (legacy region still
  // has children, or an existing map file failed to read): the caller must
  // NOT stamp the version, so the migration retries on the next boot instead
  // of being silently consumed. A missing map file is success — a fresh
  // install has no legacy region to retire.
  static bool applyDefaults(NodePrefs& prefs, RegionMap& region_map, FILESYSTEM* fs, const char* version);

  // Reads the previously-recorded firmware version string from the filesystem.
  // Empty string if none recorded yet (first boot).
  static void readVersion(FILESYSTEM* fs, char* buf, size_t bufLen);

  // Records the given firmware version string to the filesystem.
  static void writeVersion(FILESYSTEM* fs, const char* version);

 private:
  // Returns true if `version` is strictly less than `threshold`.
  // Accepts "0.0.7", "2025.3.4", optional leading 'v'/'V', and orders
  // pre-release suffixes after the 3rd numeric component:
  // 1.3.0-rc1 < 1.3.0-rc2 < 1.4.0 < 1.5.0-rc1 < 1.5.0.
  // Empty/null version is treated as oldest.
  static bool versionLessThan(const char* version, const char* threshold);
};
