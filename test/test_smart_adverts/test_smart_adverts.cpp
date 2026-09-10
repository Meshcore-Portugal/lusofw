// SmartAdverts (src/lusofw/SmartAdverts.h) — host tests against the REAL
// header with a real SHA-256 (test/shims). Covers the slot math, both clock
// anchors, the MyMesh::loop scheduler model, and the fix for the
// flood_advert_interval==0 arming (commit caffd8ed).
//
// The scheduler simulation replicates MyMesh::loop()'s ENABLE_SMART_ADVERTS
// branch and updateFloodAdvertTimer() one-to-one, ticking once per second.
#include <gtest/gtest.h>

#include <cstdio>
#include <cstdint>
#include <vector>

#include <Mesh.h>
#include <lusofw/SmartAdverts.h>

namespace {

constexpr uint32_t kW = SmartAdverts::WINDOW_SIZE_SECONDS;        // 82800
constexpr int32_t kJ = SmartAdverts::JITTER_MAX_SECONDS;          // 3
constexpr uint32_t kMinValid = SmartAdverts::MIN_VALID_EPOCH;
constexpr uint64_t kWms = (uint64_t)kW * 1000ull;

uint32_t RefHash(const char* name, const uint8_t* key4) {
  uint32_t h = 0;
  const char* n = name ? name : "";
  mesh::Utils::sha256((uint8_t*)&h, sizeof(h), (const uint8_t*)n, (int)strlen(n), key4, 4);
  return h;
}

bool Sha256SelfTest() {
  uint8_t out[32];
  mesh::Utils::sha256(out, 32, (const uint8_t*)"abc", 3);
  static const uint8_t want_abc[32] = {
    0xba,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,0x41,0x41,0x40,0xde,0x5d,0xae,0x22,0x23,
    0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,0xb4,0x10,0xff,0x61,0xf2,0x00,0x15,0xad};
  if (memcmp(out, want_abc, 32) != 0) return false;
  mesh::Utils::sha256(out, 32, (const uint8_t*)"", 0);
  static const uint8_t want_empty[32] = {
    0xe3,0xb0,0xc4,0x42,0x98,0xfc,0x1c,0x14,0x9a,0xfb,0xf4,0xc8,0x99,0x6f,0xb9,0x24,
    0x27,0xae,0x41,0xe4,0x64,0x9b,0x93,0x4c,0xa4,0x95,0x99,0x1b,0x78,0x52,0xb8,0x55};
  return memcmp(out, want_empty, 32) == 0;
}

int32_t JitFor(uint32_t hash, uint64_t cycle_start) {
  return (int32_t)((hash ^ (uint32_t)cycle_start) % ((kJ * 2) + 1)) - kJ;
}

void CheckRtcInvariant(const char* name, const uint8_t* key4, uint32_t now) {
  const uint32_t h = RefHash(name, key4);
  const uint64_t offset = h % kW;
  const uint32_t w = SmartAdverts::nextAdvertWaitSeconds(name, key4, now, 12345u);
  ASSERT_GE(w, 1u);
  ASSERT_LE(w, kW + 2 * kJ);
  const uint64_t t = (uint64_t)now + w;
  ASSERT_GT(t, (uint64_t)now);
  const uint64_t cs = t - (t % kW);
  const uint64_t expected = cs + offset + (uint64_t)JitFor(h, cs);
  ASSERT_EQ(t, expected) << "now=" << now << " w=" << w;
}

// 1:1 replication of MyMesh::loop()'s ENABLE_SMART_ADVERTS branch plus
// updateFloodAdvertTimer() (with the caffd8ed interval==0 early return).
struct SimResult {
  std::vector<uint64_t> send_full_millis;
  std::vector<uint32_t> send_epoch;
};

bool MsPassed(uint32_t now, uint32_t ts) { return (int32_t)(now - ts) > 0; }

SimResult Simulate(const char* name, const uint8_t* key4, bool rtc_valid, uint32_t t0_epoch,
                   uint32_t t0_millis, uint64_t sim_seconds, uint32_t flood_interval_hours = 23) {
  SimResult r;
  uint32_t now_millis = t0_millis;
  uint32_t next_advert_check = t0_millis + 30000u;
  uint32_t next_flood_advert = 0;
  uint64_t full = t0_millis;
  for (uint64_t s = 0; s < sim_seconds; s++) {
    full += 1000;
    now_millis += 1000;
    uint32_t now_epoch = rtc_valid ? t0_epoch + (uint32_t)((full - t0_millis) / 1000) : 0u;
    if (next_advert_check && MsPassed(now_millis, next_advert_check)) {
      next_advert_check = now_millis + 1000u;
      if (flood_interval_hours > 0) {
        if (next_flood_advert == 0) {
          uint32_t wait = SmartAdverts::nextAdvertWaitSeconds(name, key4, now_epoch, now_millis);
          next_flood_advert = now_millis + wait * 1000u;
        } else if (MsPassed(now_millis, next_flood_advert)) {
          r.send_full_millis.push_back(full);
          r.send_epoch.push_back(now_epoch);
          next_flood_advert = 0;
        }
        if (next_flood_advert == 0) {
          uint32_t wait = SmartAdverts::nextAdvertWaitSeconds(name, key4, now_epoch, now_millis);
          next_flood_advert = now_millis + wait * 1000u;
        }
      }
    }
  }
  return r;
}

uint32_t Lcg() {
  static uint32_t s = 0x12345678u;
  return s = s * 1664525u + 1013904223u;
}

}  // namespace

TEST(SmartAdverts, Sha256ShimIsReal) {
  ASSERT_TRUE(Sha256SelfTest());
}

TEST(SmartAdverts, RtcSlotInvariantSweep) {
  for (int i = 0; i < 64; i++) {
    char nm[32];
    snprintf(nm, sizeof nm, "RPT-%03d-%02u", i, Lcg() % 100u);
    uint8_t key[4];
    for (int k = 0; k < 4; k++) key[k] = (uint8_t)(Lcg() >> 24);
    for (int j = 0; j < 64; j++) {
      ASSERT_NO_FATAL_FAILURE(
          CheckRtcInvariant(nm, key, kMinValid + j * 6151u + (Lcg() % 600u)));
    }
  }
}

TEST(SmartAdverts, RtcCycleBoundaries) {
  const char* nm = "Boundary-Node";
  const uint8_t key[4] = {0xDE, 0xAD, 0xBE, 0xEF};
  for (uint64_t k = (kMinValid + kW - 1) / kW; k < (kMinValid + kW - 1) / kW + 60; k++) {
    const uint32_t base = (uint32_t)(k * kW);
    ASSERT_NO_FATAL_FAILURE(CheckRtcInvariant(nm, key, base));
    ASSERT_NO_FATAL_FAILURE(CheckRtcInvariant(nm, key, base - 1));
    ASSERT_NO_FATAL_FAILURE(CheckRtcInvariant(nm, key, base + 1));
    ASSERT_NO_FATAL_FAILURE(CheckRtcInvariant(nm, key, base + kW - 1));
  }
}

TEST(SmartAdverts, RtcEpochWrap2106) {
  const char* nm = "Wrap-Node";
  const uint8_t key[4] = {1, 2, 3, 4};
  ASSERT_NO_FATAL_FAILURE(CheckRtcInvariant(nm, key, 4294967295u));
  ASSERT_NO_FATAL_FAILURE(CheckRtcInvariant(nm, key, 4294967294u));
  ASSERT_NO_FATAL_FAILURE(CheckRtcInvariant(nm, key, 4294967295u - kW));
  ASSERT_NO_FATAL_FAILURE(CheckRtcInvariant(nm, key, 4294967295u - kW + 1));
  ASSERT_NO_FATAL_FAILURE(CheckRtcInvariant(nm, key, 4294900000u));
}

TEST(SmartAdverts, FallbackWaitsBoundedBelowMinValidEpoch) {
  const char* nm = "NoRtc-Node";
  const uint8_t key[4] = {9, 9, 9, 9};
  for (int j = 0; j < 500; j++) {
    uint32_t w = SmartAdverts::nextAdvertWaitSeconds(nm, key, kMinValid - 1 - (j % 97), j * 131u);
    ASSERT_GE(w, 1u);
    ASSERT_LE(w, kW + 2 * kJ);
  }
}

TEST(SmartAdverts, BranchSwitchesAtMinValidEpoch) {
  const char* nm = "Branch-Node";
  const uint8_t key[4] = {3, 1, 4, 1};
  uint32_t below_a = SmartAdverts::nextAdvertWaitSeconds(nm, key, kMinValid - 1, 424242u);
  uint32_t below_b = SmartAdverts::nextAdvertWaitSeconds(nm, key, 1000u, 424242u);
  EXPECT_EQ(below_a, below_b);                       // pre-2026 epoch ignored
  uint32_t at = SmartAdverts::nextAdvertWaitSeconds(nm, key, kMinValid, 424242u);
  EXPECT_NE(at, below_a);                            // RTC branch engages
}

TEST(SmartAdverts, DeterministicAndNullNameSafe) {
  const uint8_t key[4] = {7, 7, 7, 7};
  EXPECT_EQ(SmartAdverts::nextAdvertWaitSeconds("Det", key, 1780000000u, 500000u),
            SmartAdverts::nextAdvertWaitSeconds("Det", key, 1780000000u, 500000u));
  EXPECT_EQ(SmartAdverts::nextAdvertWaitSeconds("Det", key, 0, 500000u),
            SmartAdverts::nextAdvertWaitSeconds("Det", key, 0, 500000u));
  EXPECT_GE(SmartAdverts::nextAdvertWaitSeconds(NULL, key, 1780000000u, 0), 1u);
}

// Documented behaviour (SmartAdverts.h): the uptime fallback clock wraps every
// ~49.7d and ONE window is irregular at the wrap. All gaps are 23h +/- 7s
// except at most one anomalous gap per wrap, which stays above one hour.
TEST(SmartAdverts, FallbackCadenceOnePerWindowIncludingMillisWrap) {
  struct Start { uint32_t millis; const char* tag; };
  const Start starts[] = {
    {0u, "boot@0"},
    {1000u, "boot@1s"},
    {(uint32_t)(kWms - 5000), "boot@window-edge"},
    {4293000000u, "boot@pre-millis-wrap"},
    {4294967295u - 300000u, "boot@millis-wrap-5min"},
  };
  for (const auto& st : starts) {
    for (int idn = 0; idn < 4; idn++) {
      char nm[48];
      snprintf(nm, sizeof nm, "Sim-%02d-%s", idn, st.tag);
      const uint8_t key[4] = {(uint8_t)idn, (uint8_t)(idn * 37), 0x5A, (uint8_t)(idn * 11)};
      SimResult r = Simulate(nm, key, false, 0, st.millis, 4 * kW + 3600);
      ASSERT_GE(r.send_full_millis.size(), 3u) << st.tag;
      size_t anomalies = 0;
      for (size_t i = 1; i < r.send_full_millis.size(); i++) {
        int64_t gap = (int64_t)(r.send_full_millis[i] - r.send_full_millis[i - 1]);
        bool regular = gap >= (int64_t)kWms - 7000 && gap <= (int64_t)kWms + 7000;
        if (!regular) {
          anomalies++;
          ASSERT_GE(gap, 3600 * 1000LL) << st.tag << " gap=" << gap;  // no storm
        }
      }
      ASSERT_LE(anomalies, 1u) << st.tag;
    }
  }
}

TEST(SmartAdverts, RtcCadenceOnePerCalendarCycle) {
  const char* nm = "Rtc-Sim";
  const uint8_t key[4] = {0x11, 0x22, 0x33, 0x44};
  const uint32_t t0 = kMinValid + 1234u;
  SimResult r = Simulate(nm, key, true, t0, 700000u, 6 * kW);
  ASSERT_GE(r.send_epoch.size(), 4u);
  const uint64_t offset = RefHash(nm, key) % kW;
  for (size_t i = 1; i < r.send_epoch.size(); i++) {
    EXPECT_EQ((uint64_t)r.send_epoch[i] / kW, (uint64_t)r.send_epoch[i - 1] / kW + 1);
  }
  for (size_t i = 0; i < r.send_epoch.size(); i++) {
    const uint64_t cs = ((uint64_t)r.send_epoch[i] / kW) * kW;
    int64_t delta = (int64_t)r.send_epoch[i] - (int64_t)(cs + offset);
    EXPECT_GE(delta, -4);
    EXPECT_LE(delta, 4);  // jitter +/-3s + 1s scheduler tick
  }
}

// Commit caffd8ed: with flood_advert_interval == 0 the smart branch must leave
// the timer disarmed (mirrors the legacy branch); no sends ever occur.
TEST(SmartAdverts, FloodIntervalZeroDisablesAdverts) {
  SimResult r = Simulate("Off", (const uint8_t*)"\1\2\3\4", false, 0, 0, 3 * kW, 0);
  EXPECT_TRUE(r.send_full_millis.empty());
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
