// AutoRegions (src/lusofw/AutoRegions.{h,cpp} + compiled region geometry +
// real RegionMap persistence) — host tests against the REAL sources.
//
// checkRegionAutoAssign keeps its evaluation state in file-statics, so the
// scenarios form one ordered flow: every step changes the node name,
// coordinates or location policy relative to the previous step, which forces a
// re-evaluation. Helpers ReevaluateAt()/ReevaluateNamed() make that explicit.
//
// Covers the radio regulation derivation incl. the PA-gain fix (5b9f53e3) and
// the LORA_TX_POWER cap. PA-gain and low-power-cap variants of this suite run
// in [env:native_lusofw_pa] / [env:native_lusofw_cap10].
#include <gtest/gtest.h>

#include <cstring>
#include <set>
#include <string>
#include <vector>

#include <Arduino.h>
#include <helpers/TxtDataHelpers.h>
#include <lusofw/AutoRegions.h>

#ifndef LORA_TX_POWER
#define LORA_TX_POWER 22
#endif
#ifndef LUSOFW_TX_PA_GAIN
#define LUSOFW_TX_PA_GAIN 0
#endif

namespace {

struct Ctx {
  TransportKeyStore tks;
  RegionMap rm;
  FILESYSTEM fs;
  NodePrefs prefs;
  Ctx() : rm(tks) {
    prefs.airtime_factor = 1.0f;
    prefs.tx_power_dbm = LORA_TX_POWER;
    prefs.advert_loc_policy = ADVERT_LOC_PREFS;
    prefs.freq = 869.618f;
    prefs.radio_manual = 0;
    StrHelper::strncpy(prefs.node_name, "Heltec_T114 Repeater", sizeof(prefs.node_name));
  }
};

std::set<std::string> Names(const RegionMap& rm) {
  std::set<std::string> out;
  for (int i = 0; i < rm.getCount(); i++) out.insert(rm.getByIdx(i)->name);
  return out;
}

const RegionEntry* Find(const RegionMap& rm, const char* n) {
  for (int i = 0; i < rm.getCount(); i++) {
    if (strcmp(rm.getByIdx(i)->name, n) == 0) return rm.getByIdx(i);
  }
  return nullptr;
}

bool HasAll(const RegionMap& rm, std::initializer_list<const char*> want) {
  auto ns = Names(rm);
  for (const char* w : want) {
    if (ns.count(w) == 0) return false;
  }
  return true;
}

void ReevaluateAt(Ctx& c, double lat, double lon) {
  c.prefs.node_lat = lat;
  c.prefs.node_lon = lon;
  AutoRegions::checkRegionAutoAssign(c.rm, c.prefs, &c.fs);
}

// The EU conducted limit for a band, as derived by applyRadioRegulation in
// this build: limit minus the variant PA gain, floored at 0, capped by the
// board max. Keeps the flow tests valid in the _pa / _cap10 env variants.
int8_t ExpectedEuTx(int limit_dbm) {
  int v = limit_dbm - LUSOFW_TX_PA_GAIN;
  if (v < 0) v = 0;
  if (v > LORA_TX_POWER) v = LORA_TX_POWER;
  return (int8_t)v;
}

void AssertTreeOk(RegionMap& rm, const char* tag) {
  for (int i = 0; i < rm.getCount(); i++) {
    const RegionEntry* r = rm.getByIdx(i);
    int hops = 0;
    while (r && r->id != 0 && hops++ < 40) {
      const RegionEntry* p = (r->parent == 0) ? &rm.getWildcard() : rm.findById(r->parent);
      ASSERT_NE(p, nullptr) << tag << ": parent " << r->parent << " of " << r->name << " missing";
      r = p;
    }
    ASSERT_LE(hops, 40) << tag << ": hierarchy walk terminates for " << rm.getByIdx(i)->name;
  }
}

struct Probe { double lat, lon; const char* district; const char* macro; };

const Probe kDistrictProbes[] = {
  {40.6405, -8.6538, "#pt-aveiro", "#pt-centro"},
  {38.0200, -7.8700, "#pt-beja", "#pt-alentejo"},
  {41.5454, -8.4265, "#pt-braga", "#pt-norte"},
  {41.8058, -6.7570, "#pt-braganca", "#pt-norte"},
  {39.8222, -7.4900, "#pt-castelo-branco", "#pt-centro"},
  {40.2112, -8.4290, "#pt-coimbra", "#pt-centro"},
  {38.5714, -7.9135, "#pt-evora", "#pt-alentejo"},
  {37.0194, -7.9304, "#pt-faro", "#pt-algarve"},
  {40.5377, -7.2660, "#pt-guarda", "#pt-centro"},
  {39.7495, -8.8080, "#pt-leiria", "#pt-centro"},
  {38.7223, -9.1393, "#pt-lisboa", "#pt-lisboa-vale-do-tejo"},
  {39.2946, -7.4310, "#pt-portalegre", "#pt-alentejo"},
  {41.1496, -8.6109, "#pt-porto", "#pt-norte"},
  {39.2334, -8.6860, "#pt-santarem", "#pt-lisboa-vale-do-tejo"},
  {38.5243, -8.8882, "#pt-setubal", "#pt-lisboa-vale-do-tejo"},
  {41.6947, -8.8321, "#pt-viana-do-castelo", "#pt-norte"},
  {41.3006, -7.7441, "#pt-vila-real", "#pt-norte"},
  {40.6571, -7.9142, "#pt-viseu", "#pt-centro"},
  {32.6655, -16.9255, "#pt-ilha-da-madeira", "#pt-madeira"},
  {33.0630, -16.3380, "#pt-ilha-de-porto-santo", "#pt-madeira"},
  {36.9680, -25.1000, "#pt-ilha-de-santa-maria", "#pt-acores"},
  {37.7762, -25.4966, "#pt-ilha-de-sao-miguel", "#pt-acores"},
  {38.7200, -27.2200, "#pt-ilha-terceira", "#pt-acores"},
  {39.0520, -28.0100, "#pt-ilha-da-graciosa", "#pt-acores"},
  {38.6470, -28.0660, "#pt-ilha-de-sao-jorge", "#pt-acores"},
  {38.4650, -28.4040, "#pt-ilha-do-pico", "#pt-acores"},
  {38.5770, -28.7100, "#pt-ilha-do-faial", "#pt-acores"},
  {39.4550, -31.1270, "#pt-ilha-das-flores", "#pt-acores"},
  {39.6990, -31.1100, "#pt-ilha-do-corvo", "#pt-acores"},
};
constexpr size_t kNumProbes = sizeof(kDistrictProbes) / sizeof(kDistrictProbes[0]);

struct EuroProbe { double lat, lon; bool in_europe; const char* name; };
const EuroProbe kEuroProbes[] = {
  {35.8890, -5.3160, true, "ceuta"},
  {35.7700, -5.8000, false, "tangier"},
  {28.1100, -15.4400, true, "canary"},
  {33.5700, -7.5900, false, "casablanca"},
  {64.1500, -21.9400, true, "reykjavik"},
  {37.7800, -25.6700, true, "azores"},
  {40.4168, -3.7038, true, "madrid"},
  {0.0000, 0.0000, false, "null-island"},
};

}  // namespace

TEST(AutoRegionsFlow, 01_DefaultT114Config) {
  Ctx c;
  AutoRegions::checkRegionAutoAssign(c.rm, c.prefs, &c.fs);
  EXPECT_FALSE(AutoRegions::isNodeInEurope());
  EXPECT_EQ(c.rm.getCount(), 0);
  EXPECT_EQ(c.prefs.tx_power_dbm, LORA_TX_POWER);
  EXPECT_FLOAT_EQ(c.prefs.airtime_factor, 1.0f);
  EXPECT_EQ(c.fs.writes, 0u);  // no flash write when the map is unchanged
  AutoRegions::checkRegionAutoAssign(c.rm, c.prefs, &c.fs);
  EXPECT_EQ(c.fs.writes, 0u);  // idempotent
}

TEST(AutoRegionsFlow, 02_NamePrefixFallback) {
  Ctx c;
  StrHelper::strncpy(c.prefs.node_name, "LI.Repeater", sizeof(c.prefs.node_name));
  AutoRegions::checkRegionAutoAssign(c.rm, c.prefs, &c.fs);
  EXPECT_EQ(c.rm.getCount(), 4);
  EXPECT_TRUE(HasAll(c.rm, {"#pt-lisboa", "#pt-lisboa-vale-do-tejo", "#pt", "#europe"}));
  EXPECT_EQ(c.prefs.tx_power_dbm, ExpectedEuTx(27));  // EU 869.618 sub-band (27 dBm limit)
  // Coordinates present but policy NONE: prefix still wins, coords ignored
  Ctx c2;
  c2.prefs.node_lat = 41.1496; c2.prefs.node_lon = -8.6109;  // Porto coords
  c2.prefs.advert_loc_policy = ADVERT_LOC_NONE;
  StrHelper::strncpy(c2.prefs.node_name, "LI.Repeater", sizeof(c2.prefs.node_name));
  AutoRegions::checkRegionAutoAssign(c2.rm, c2.prefs, &c2.fs);
  EXPECT_TRUE(HasAll(c2.rm, {"#pt-lisboa", "#pt-lisboa-vale-do-tejo"}));
}

TEST(AutoRegionsFlow, 03_CoordinatesAssignHierarchyAndPersist) {
  Ctx c;
  c.prefs.freq = 433.375f;  // audit bench parameters
  ReevaluateAt(c, 38.7223, -9.1393);  // Lisbon
  EXPECT_EQ(c.rm.getCount(), 4);
  EXPECT_TRUE(HasAll(c.rm, {"#europe", "#pt", "#pt-lisboa", "#pt-lisboa-vale-do-tejo"}));
  EXPECT_TRUE(AutoRegions::isNodeInEurope());
  EXPECT_EQ(c.prefs.tx_power_dbm, ExpectedEuTx(10));  // EU 433 MHz conducted limit
  EXPECT_FLOAT_EQ(c.prefs.airtime_factor, 9.0f);
  const RegionEntry* eu = Find(c.rm, "#europe");
  const RegionEntry* pt = Find(c.rm, "#pt");
  ASSERT_NE(eu, nullptr);
  ASSERT_NE(pt, nullptr);
  EXPECT_EQ(eu->parent, 0);
  EXPECT_TRUE(eu->flags & REGION_AUTO_ASSIGN);
  EXPECT_EQ(Find(c.rm, "#pt-lisboa")->parent, pt->id);
  EXPECT_EQ(Find(c.rm, "#pt-lisboa-vale-do-tejo")->parent, pt->id);
  EXPECT_EQ(pt->parent, eu->id);
  EXPECT_FALSE(Find(c.rm, "#pt-lisboa")->flags & REGION_DENY_FLOOD);
  EXPECT_EQ(c.fs.writes, 1u);
  AssertTreeOk(c.rm, "lisbon");

  {  // persistence round-trip
    TransportKeyStore tks2;
    RegionMap rm2(tks2);
    ASSERT_TRUE(rm2.load(&c.fs));
    EXPECT_EQ(Names(rm2), Names(c.rm));
  }
  const std::vector<uint8_t>* before = c.fs.bytes("/regions2");
  AutoRegions::checkRegionAutoAssign(c.rm, c.prefs, &c.fs);  // no-op re-eval
  EXPECT_EQ(c.fs.writes, 1u);
  ASSERT_NE(before, nullptr);
  ASSERT_NE(c.fs.bytes("/regions2"), nullptr);
  EXPECT_EQ(*before, *c.fs.bytes("/regions2"));  // byte-identical map

  // Move to Porto: district/macro swap, country ids stable, one re-save
  const uint16_t pt_id = pt->id, eu_id = eu->id;
  ReevaluateAt(c, 41.1496, -8.6109);
  EXPECT_EQ(c.rm.getCount(), 4);
  EXPECT_TRUE(HasAll(c.rm, {"#europe", "#pt", "#pt-porto", "#pt-norte"}));
  EXPECT_EQ(Find(c.rm, "#pt")->id, pt_id);
  EXPECT_EQ(Find(c.rm, "#europe")->id, eu_id);
  EXPECT_EQ(c.fs.writes, 2u);
  AssertTreeOk(c.rm, "porto");

  // radio_manual freezes derived radio state, regions still reassign
  c.prefs.radio_manual = 1;
  AutoRegions::applyRadioRegulation(c.prefs, 869.5f);
  const int8_t frozen_tx = c.prefs.tx_power_dbm;  // derived before the latch
  EXPECT_EQ(frozen_tx, ExpectedEuTx(10));         // 433 value stays frozen
  ReevaluateAt(c, 38.7223, -9.1393);
  EXPECT_TRUE(HasAll(c.rm, {"#pt-lisboa", "#pt-lisboa-vale-do-tejo"}));
  EXPECT_EQ(c.prefs.tx_power_dbm, frozen_tx);
}

TEST(AutoRegionsFlow, 04_RenameSweepsAutoRegions) {
  Ctx c;
  StrHelper::strncpy(c.prefs.node_name, "PO.Alpha", sizeof(c.prefs.node_name));
  AutoRegions::checkRegionAutoAssign(c.rm, c.prefs, &c.fs);
  EXPECT_TRUE(HasAll(c.rm, {"#pt-porto", "#pt-norte", "#pt", "#europe"}));
  StrHelper::strncpy(c.prefs.node_name, "ZZ.Nowhere", sizeof(c.prefs.node_name));
  AutoRegions::checkRegionAutoAssign(c.rm, c.prefs, &c.fs);
  EXPECT_EQ(c.rm.getCount(), 0);
  EXPECT_FALSE(AutoRegions::isNodeInEurope());
  EXPECT_EQ(c.prefs.tx_power_dbm, LORA_TX_POWER);
}

TEST(AutoRegionsFlow, 05_EuropeOutsidePtGetsEuropeOnly) {
  Ctx c;
  ReevaluateAt(c, 40.4168, -3.7038);  // Madrid
  EXPECT_TRUE(AutoRegions::isNodeInEurope());
  ASSERT_EQ(c.rm.getCount(), 1);
  EXPECT_TRUE(HasAll(c.rm, {"#europe"}));
  EXPECT_EQ(c.prefs.tx_power_dbm, ExpectedEuTx(27));
  EXPECT_FLOAT_EQ(c.prefs.airtime_factor, 9.0f);
}

TEST(AutoRegionsFlow, 06_OutsideEuropeBoardMax) {
  Ctx c;
  ReevaluateAt(c, 40.7128, -74.0060);  // NYC
  EXPECT_FALSE(AutoRegions::isNodeInEurope());
  EXPECT_EQ(c.rm.getCount(), 0);
  EXPECT_EQ(c.prefs.tx_power_dbm, LORA_TX_POWER);
  EXPECT_FLOAT_EQ(c.prefs.airtime_factor, 1.0f);
  EXPECT_EQ(c.fs.writes, 0u);
}

#if LUSOFW_TX_PA_GAIN == 0
TEST(AutoRegionsFlow, 07_EuRegulationBandTable) {
  Ctx c;
  ReevaluateAt(c, 38.7223, -9.1393);
  struct Row { float f; int limit; };
  const Row rows[] = {
    {869.40f, 27}, {869.65f, 27}, {869.50f, 27},
    {869.3999f, 14}, {869.66f, 14}, {863.0f, 14}, {870.0f, 14}, {868.0f, 14},
    {862.99f, 10}, {433.05f, 10}, {433.375f, 10}, {434.79f, 10}, {434.80f, 10},
    {150.0f, 10}, {915.0f, 10}, {2450.0f, 10},
  };
  for (const auto& row : rows) {
    AutoRegions::applyRadioRegulation(c.prefs, row.f);
    EXPECT_EQ(c.prefs.tx_power_dbm, ExpectedEuTx(row.limit))
        << "EU freq " << row.f << " -> " << (int)c.prefs.tx_power_dbm;
    EXPECT_FLOAT_EQ(c.prefs.airtime_factor, 9.0f);
  }
}

TEST(AutoRegionsFlow, 08_OutsideEuRegulationUnchanged) {
  Ctx c;
  ReevaluateAt(c, 40.7128, -74.0060);
  for (float f : {869.5f, 433.375f, 915.0f}) {
    AutoRegions::applyRadioRegulation(c.prefs, f);
    EXPECT_EQ(c.prefs.tx_power_dbm, LORA_TX_POWER);
    EXPECT_FLOAT_EQ(c.prefs.airtime_factor, 1.0f);
  }
}
#endif

#if LUSOFW_TX_PA_GAIN == 10
// Fix 5b9f53e3 (audit C-1): EU conducted limits minus the variant PA gain,
// floored at 0; outside Europe the board max applies with no subtraction.
TEST(AutoRegionsFlow, 07_EuRegulationSubtractsPaGain) {
  Ctx c;
  ReevaluateAt(c, 38.7223, -9.1393);
  struct Row { float f; int8_t tx; };
  const Row rows[] = {
    {869.40f, 17}, {869.65f, 17}, {869.50f, 17},
    {869.3999f, 4}, {867.5f, 4}, {863.0f, 4}, {870.0f, 4},
    {862.99f, 0}, {433.05f, 0}, {433.375f, 0}, {434.79f, 0},
    {434.80f, 0}, {915.0f, 0}, {150.0f, 0},
  };
  for (const auto& row : rows) {
    AutoRegions::applyRadioRegulation(c.prefs, row.f);
    EXPECT_EQ(c.prefs.tx_power_dbm, row.tx)
        << "EU+PA10 freq " << row.f << " -> " << (int)c.prefs.tx_power_dbm;
    EXPECT_FLOAT_EQ(c.prefs.airtime_factor, 9.0f);
  }
  Ctx c2;
  ReevaluateAt(c2, 40.7128, -74.0060);
  for (float f : {869.5f, 433.375f}) {
    AutoRegions::applyRadioRegulation(c2.prefs, f);
    EXPECT_EQ(c2.prefs.tx_power_dbm, LORA_TX_POWER);  // no EU limit to subtract
  }
}
#else
TEST(AutoRegionsFlow, 07_EuRegulationSubtractsPaGain) {
  GTEST_SKIP() << "PA-gain table runs in [env:native_lusofw_pa]";
}
#endif

#if LORA_TX_POWER == 10
TEST(AutoRegionsFlow, 09_LowPowerBoardCap) {
  Ctx c;
  ReevaluateAt(c, 38.7223, -9.1393);
  EXPECT_EQ(c.prefs.tx_power_dbm, 10);  // caps the 869.4-869.65 sub-band
  AutoRegions::applyRadioRegulation(c.prefs, 433.375f);
  EXPECT_EQ(c.prefs.tx_power_dbm, 10);
}
#else
TEST(AutoRegionsFlow, 09_LowPowerBoardCap) {
  GTEST_SKIP() << "LORA_TX_POWER cap runs in [env:native_lusofw_cap10]";
}
#endif

TEST(AutoRegionsFlow, 10_FullMapDegradesGracefully) {
  Ctx c;
  // distinct name: forces a re-evaluation regardless of which regulation
  // variants ran (or were skipped) before this test
  StrHelper::strncpy(c.prefs.node_name, "MF.Node", sizeof(c.prefs.node_name));
  for (int i = 0; i < 32; i++) {
    char nm[16];
    snprintf(nm, sizeof nm, "user%02d", i);
    ASSERT_NE(c.rm.putRegion(nm, 0), nullptr);
  }
  ReevaluateAt(c, 38.7223, -9.1393);
  EXPECT_EQ(c.rm.getCount(), 32);  // stays at MAX_REGION_ENTRIES
  EXPECT_NE(Find(c.rm, "user00"), nullptr);
  EXPECT_NE(Find(c.rm, "user31"), nullptr);
  EXPECT_TRUE(AutoRegions::isNodeInEurope());
  AssertTreeOk(c.rm, "mapfull");
}

TEST(AutoRegionsFlow, 11_UserRegionNameClashSemantics) {
  // A user region named like an auto region is re-parented and flood-enabled,
  // but NOT re-flagged REGION_AUTO_ASSIGN (inject_hierarchy only flags new
  // entries), so it survives the later sweep.
  Ctx c;
  StrHelper::strncpy(c.prefs.node_name, "CL.Node", sizeof(c.prefs.node_name));
  ASSERT_NE(c.rm.putRegion("pt", 0), nullptr);
  ReevaluateAt(c, 38.7223, -9.1393);
  const RegionEntry* u = Find(c.rm, "pt");
  ASSERT_NE(u, nullptr);
  EXPECT_FALSE(u->flags & REGION_AUTO_ASSIGN);
  EXPECT_FALSE(u->flags & REGION_DENY_FLOOD);
  const RegionEntry* eu = Find(c.rm, "#europe");
  EXPECT_EQ(u->parent, (eu ? eu->id : 0));
  AssertTreeOk(c.rm, "clash");
  ReevaluateAt(c, 40.7128, -74.0060);
  EXPECT_NE(Find(c.rm, "pt"), nullptr);  // unflagged: survives the sweep
  AssertTreeOk(c.rm, "clash-after-move");
}

TEST(AutoRegionsFlow, 12_DistrictProbesSingleMatch) {
  Ctx c;
  StrHelper::strncpy(c.prefs.node_name, "PR.Node", sizeof(c.prefs.node_name));
  c.prefs.freq = 433.375f;
  for (size_t i = 0; i < kNumProbes; i++) {
    const Probe& p = kDistrictProbes[i];
    ReevaluateAt(c, p.lat, p.lon);
    std::set<std::string> districts, macros;
    for (size_t d = 0; d < kNumProbes; d++) {
      if (Find(c.rm, kDistrictProbes[d].district)) districts.insert(kDistrictProbes[d].district);
      if (Find(c.rm, kDistrictProbes[d].macro)) macros.insert(kDistrictProbes[d].macro);
    }
    EXPECT_EQ(districts.size(), 1u) << p.district << " at (" << p.lat << "," << p.lon << ")";
    EXPECT_EQ(macros.size(), 1u) << p.district;
    EXPECT_NE(Find(c.rm, p.district), nullptr) << p.district;
    EXPECT_NE(Find(c.rm, p.macro), nullptr) << p.district;
    EXPECT_TRUE(AutoRegions::isNodeInEurope()) << p.district;
    AssertTreeOk(c.rm, p.district);
  }
}

TEST(AutoRegionsFlow, 13_EuropeMembershipEdges) {
  Ctx c;
  for (const auto& p : kEuroProbes) {
    ReevaluateAt(c, p.lat, p.lon);
    EXPECT_EQ(AutoRegions::isNodeInEurope(), p.in_europe) << p.name;
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
