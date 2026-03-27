#pragma once

#include <helpers/CommonCLI.h>

class FirmwareMigration {
 private:
  struct VersionEntry {
    const char* version;
    void (*defaults)(NodePrefs& prefs);
  };

  static const VersionEntry s_versions[];

  static int findVersionIndex(const char* version);

 public:
  static void applyDefaultsByVersion(const char* oldVersion, const char* newVersion,
                                     NodePrefs& prefs);

  static void readVersion(FILESYSTEM* fs, char* buf, size_t bufLen);
  static void writeVersion(FILESYSTEM* fs, const char* version);
};
