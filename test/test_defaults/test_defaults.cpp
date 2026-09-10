// LusoDefaults (src/lusofw/Defaults.{h,cpp}) — host tests against the REAL
// source: version ordering (incl. the saturating digit accumulation from
// commit a567d332), baseline defaults, the radio_manual latch bootstrap gated
// below v2026.9.1-rc4 (commit 7535f9fa), and the #portugal retirement
// migration including blocked/retry semantics.
#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include <Arduino.h>
#include <helpers/TxtDataHelpers.h>

#define private public  // test access to LusoDefaults::versionLessThan
#include <lusofw/Defaults.h>
#undef private

#ifndef LORA_TX_POWER
#define LORA_TX_POWER 22
#endif

namespace {

struct VCase { const char* v; const char* t; bool want; };

NodePrefs MakePrefs() {
  NodePrefs prefs;
  StrHelper::strncpy(prefs.node_name, "T", sizeof(prefs.node_name));
  prefs.airtime_factor = 1.0f;
  prefs.tx_power_dbm = LORA_TX_POWER;
  return prefs;
}

}  // namespace

TEST(Defaults, VersionOrdering) {
  const VCase vcases[] = {
    {"", "2026.9.1", true},
    {nullptr, "2026.9.1", true},
    {"2026.9.1", "2026.9.1", false},
    {"2026.9.1", "2026.9.2", true},
    {"2026.10.0", "2026.9.1", false},
    {"2026.9.1-rc1", "2026.9.1", true},      // rc sorts before final
    {"2026.9.1-rc2", "2026.9.1-rc10", true}, // rc counter compared numerically
    {"2026.9.1-rc10", "2026.9.1-rc2", false},
    {"2026.9.1", "2026.9.1-rc1", false},
    {"v2026.9.1", "2026.9.1", false},        // leading v/V tolerated
    {"1.17.1", "2026.7.1", true},
    {"2026.9.1-rc4", "2026.9.1-rc4", false},
    // Fix a567d332 (audit C-18): digit runs saturate instead of UB. A 10+
    // digit run parses to a bounded value that still compares consistently.
    {"9999999999.1.1", "2026.9.1", false},       // saturated >= threshold
    {"9999999999.1.1", "9999999999.1.1", false}, // equal saturations compare equal
    {"9999999999.1.1", "9999999999.1.2", true},  // later component still orders
    {"1.0.0-99999999999", "1.0.0", true},        // saturated rc still < final
  };
  for (const auto& c : vcases) {
    bool got = LusoDefaults::versionLessThan(c.v, c.t);
    EXPECT_EQ(got, c.want) << (c.v ? c.v : "(null)") << " < " << c.t;
  }
}

TEST(Defaults, BaselineOnFreshInstall) {
  TransportKeyStore tks;
  RegionMap rm(tks);
  FILESYSTEM fs;
  NodePrefs prefs = MakePrefs();
  EXPECT_TRUE(LusoDefaults::applyDefaults(prefs, rm, &fs, ""));
  EXPECT_EQ(prefs.advert_interval, 0);
  EXPECT_EQ(prefs.flood_advert_interval, 23);
  EXPECT_EQ(prefs.path_hash_mode, 1);
  EXPECT_EQ(prefs.loop_detect, LOOP_DETECT_MINIMAL);
  EXPECT_EQ(prefs.cad_enabled, 1);
  EXPECT_EQ(prefs.advert_loc_policy, ADVERT_LOC_PREFS);
  EXPECT_EQ(prefs.radio_manual, 0);  // pristine prefs do not latch
}

// Fix 7535f9fa (audit C-3): the latch bootstrap only runs for prefs that
// predate v2026.9.1-rc4 (before the latch field existed). Values written by
// the AutoRegions derivation on rc4/rc5 must not be treated as user-set.
TEST(Defaults, LatchBootstrapGatedBelowRc4) {
  {  // rc5 node with the AutoRegions-derived af=9.0 persisted: must NOT latch
    TransportKeyStore tks; RegionMap rm(tks); FILESYSTEM fs;
    NodePrefs prefs = MakePrefs();
    prefs.airtime_factor = 9.0f;
    EXPECT_TRUE(LusoDefaults::applyDefaults(prefs, rm, &fs, "2026.9.1-rc5"));
    EXPECT_EQ(prefs.radio_manual, 0) << "derived af=9.0 from rc5 must not latch";
  }
  {  // a latch genuinely set on rc4/rc5 survives the upgrade (authoritative)
    TransportKeyStore tks; RegionMap rm(tks); FILESYSTEM fs;
    NodePrefs prefs = MakePrefs();
    prefs.airtime_factor = 9.0f;  // value equals the derived one
    prefs.radio_manual = 1;       // but the user latched it on rc5
    EXPECT_TRUE(LusoDefaults::applyDefaults(prefs, rm, &fs, "2026.9.1-rc5"));
    EXPECT_EQ(prefs.radio_manual, 1) << "genuine rc5 latch must survive";
  }
  {  // pre-rc4 prefs without the field still bootstrap from values
    TransportKeyStore tks; RegionMap rm(tks); FILESYSTEM fs;
    NodePrefs prefs = MakePrefs();
    prefs.airtime_factor = 3.0f;  // genuinely user-customized duty
    EXPECT_TRUE(LusoDefaults::applyDefaults(prefs, rm, &fs, "2026.8.1"));
    EXPECT_EQ(prefs.radio_manual, 1);
    EXPECT_FLOAT_EQ(prefs.airtime_factor, 3.0f);  // not in the baseline
  }
  {  // pre-rc4 pristine prefs: no latch
    TransportKeyStore tks; RegionMap rm(tks); FILESYSTEM fs;
    NodePrefs prefs = MakePrefs();
    EXPECT_TRUE(LusoDefaults::applyDefaults(prefs, rm, &fs, "2026.8.1"));
    EXPECT_EQ(prefs.radio_manual, 0);
  }
  {  // the heuristic never re-runs for rc4+ versions (one-shot)
    TransportKeyStore tks; RegionMap rm(tks); FILESYSTEM fs;
    NodePrefs prefs = MakePrefs();
    prefs.airtime_factor = 5.0f;
    prefs.radio_manual = 0;
    EXPECT_TRUE(LusoDefaults::applyDefaults(prefs, rm, &fs, "2026.9.1"));
    EXPECT_EQ(prefs.radio_manual, 0);
  }
}

TEST(Defaults, PortugalRetirementMigration) {
  {  // blocked: legacy region still has a child -> false, file untouched
    TransportKeyStore tks; RegionMap rm(tks); FILESYSTEM fs;
    auto* pt = rm.putRegion("#portugal", 0);
    rm.putRegion("#child", pt->id);
    rm.save(&fs);
    size_t writes_before = fs.writes;
    NodePrefs prefs = MakePrefs();
    EXPECT_FALSE(LusoDefaults::applyDefaults(prefs, rm, &fs, "2026.8.1"));
    EXPECT_EQ(fs.writes, writes_before);
    EXPECT_NE(rm.findByName("#portugal"), nullptr);  // kept for retry
  }
  {  // succeeds when childless
    TransportKeyStore tks; RegionMap rm(tks); FILESYSTEM fs;
    rm.putRegion("#portugal", 0);
    rm.save(&fs);
    NodePrefs prefs = MakePrefs();
    EXPECT_TRUE(LusoDefaults::applyDefaults(prefs, rm, &fs, "2026.8.1"));
    EXPECT_EQ(rm.findByName("#portugal"), nullptr);
  }
  {  // missing map file is success (fresh install)
    TransportKeyStore tks; RegionMap rm(tks); FILESYSTEM fs;
    NodePrefs prefs = MakePrefs();
    EXPECT_TRUE(LusoDefaults::applyDefaults(prefs, rm, &fs, "2026.8.1"));
  }
  {  // unreadable map: false (retry next boot)
    TransportKeyStore tks; RegionMap rm(tks); FILESYSTEM fs;
    fs.open("/regions2", FILE_O_WRITE).write((const uint8_t*)"garbage-not-a-map", 17);
    NodePrefs prefs = MakePrefs();
    EXPECT_FALSE(LusoDefaults::applyDefaults(prefs, rm, &fs, "2026.8.1"));
  }
  {  // map without the legacy region: nothing to do
    TransportKeyStore tks; RegionMap rm(tks); FILESYSTEM fs;
    rm.putRegion("#pt-lisboa", 0);
    rm.save(&fs);
    NodePrefs prefs = MakePrefs();
    EXPECT_TRUE(LusoDefaults::applyDefaults(prefs, rm, &fs, "2026.8.1"));
    EXPECT_NE(rm.findByName("#pt-lisboa"), nullptr);
  }
}

TEST(Defaults, VersionStampRoundTrip) {
  FILESYSTEM fs;
  LusoDefaults::writeVersion(&fs, "2026.9.1-rc5\n");
  char buf[32];
  LusoDefaults::readVersion(&fs, buf, sizeof buf);
  EXPECT_STREQ(buf, "2026.9.1-rc5");
  FILESYSTEM fs2;
  LusoDefaults::readVersion(&fs2, buf, sizeof buf);
  EXPECT_EQ(buf[0], 0);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
