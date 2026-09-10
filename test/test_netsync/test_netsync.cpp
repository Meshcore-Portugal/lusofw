// NetTimeSync (src/lusofw/NetTimeSync.{h,cpp}) — host tests against the REAL
// sources (plus the real AdvertDataHelpers). The RAM high-water mark is
// file-static and shared by every test in this binary, so the tests form one
// ordered flow: pure-rejection cases first (they never touch the mark), then
// the accept/replay chain with monotonically increasing timestamps, then the
// 2^31 straddle case last (it anchors the mark near the epoch wrap).
//
// Covers the fix for the packed-path_len hop gate (commit d1ad64b6) and the
// unsigned applied-diff computation (commit 7b746af7).
#include <gtest/gtest.h>

#include <cstring>

#include <Mesh.h>
#include <Packet.h>
#include <RateLimiter.h>
#include <RTClib.h>
#include <lusofw/NetTimeSync.h>

namespace {

constexpr uint32_t kMinPl = 1767225600u;  // 1 Jan 2026, matches NetTimeSync.cpp

struct TestClock : mesh::RTCClock {
  uint32_t t = 0;
  bool set_called = false;
  uint32_t getCurrentTime() override { return t; }
  void setCurrentTime(uint32_t v) override { t = v; set_called = true; }
};

const uint8_t kTimekeeper[PUB_KEY_SIZE] = {
  0x01, 0xB2, 0xF5, 0xDA, 0x46, 0xBC, 0x0A, 0x9C, 0x67, 0xFB, 0x8E, 0xDC, 0x36, 0x62, 0x57, 0xB6,
  0x04, 0x52, 0x73, 0xB8, 0x9F, 0x37, 0xF3, 0x08, 0x04, 0x4A, 0xD5, 0x57, 0x17, 0x34, 0xD4, 0x62};
const uint8_t kOther[PUB_KEY_SIZE] = { 0x02, 0x00, 0x00, 0x00 };

bool Call(const uint8_t* key, uint32_t ts, const uint8_t* app, size_t app_len, int hop_count,
          TestClock& clk) {
  mesh::Identity id(key);
  return NetTimeSync::handleTimekeeperAdvert(id, ts, app, app_len, hop_count, clk);
}

}  // namespace

// Rejection paths that never touch the high-water mark. Must run before the
// ordered accept chain below (declaration order within the suite is execution
// order in gtest).
TEST(Timekeeper, InitialHighWaterIsZero) {
  EXPECT_EQ(NetTimeSync::getHighWater(), 0u);
}

TEST(Timekeeper, RejectsNonTimekeeperIdentity) {
  TestClock clk;
  const uint8_t none[1] = { 0x00 };
  EXPECT_FALSE(Call(kOther, kMinPl, none, 1, 0, clk));
  EXPECT_FALSE(clk.set_called);
}

TEST(Timekeeper, RejectsNonNoneAdvertType) {
  TestClock clk;
  const uint8_t repeater[2] = { 0x02, 0x00 };      // ADV_TYPE_REPEATER
  static uint8_t latlon[MAX_ADVERT_DATA_SIZE] = {};  // LATLON flag, no body
  latlon[0] = 0x10;
  EXPECT_FALSE(Call(kTimekeeper, kMinPl, repeater, 2, 0, clk));
  EXPECT_FALSE(Call(kTimekeeper, kMinPl, latlon, 1, 0, clk));
  EXPECT_FALSE(clk.set_called);
}

TEST(Timekeeper, RejectsEmptyAppData) {
  TestClock clk;
  const uint8_t none[1] = { 0x00 };
  EXPECT_FALSE(Call(kTimekeeper, kMinPl, none, 0, 0, clk));
  EXPECT_FALSE(clk.set_called);
}

TEST(Timekeeper, RejectsPre2026Timestamps) {
  TestClock clk;
  const uint8_t none[1] = { 0x00 };
  EXPECT_FALSE(Call(kTimekeeper, kMinPl - 1, none, 1, 0, clk));
  EXPECT_FALSE(Call(kTimekeeper, 0, none, 1, 0, clk));
  EXPECT_EQ(NetTimeSync::getHighWater(), 0u);
}

// Ordered accept/replay chain: timestamps increase monotonically.
TEST(Timekeeper, HopGateUsesHopCountNotPackedPathLen) {
  // Fix d1ad64b6: the parameter is a plain hop count (the firmware caller now
  // passes packet->getPathHashCount()), so acceptance depends on hops only.
  TestClock clk;
  const uint8_t none[1] = { 0x00 };
  uint32_t ts = kMinPl;
  for (int hops = 0; hops < 10; hops++) {
    EXPECT_TRUE(Call(kTimekeeper, ts++, none, 1, hops, clk)) << "hops=" << hops;
  }
  for (int hops : {10, 11, 63, 64, 100}) {
    EXPECT_FALSE(Call(kTimekeeper, ts++, none, 1, hops, clk)) << "hops=" << hops;
  }
}

TEST(Timekeeper, PathLenPackUnpackContract) {
  // The caller-side decoding the fix relies on: setPathHashSizeAndCount packs
  // ((sz-1)<<6)|count and getPathHashCount() unpacks the count for every size.
  for (int sz = 1; sz <= 3; sz++) {
    for (int hops = 0; hops <= 63; hops++) {
      mesh::Packet pkt;
      pkt.setPathHashSizeAndCount((uint8_t)sz, (uint8_t)hops);
      EXPECT_EQ(pkt.getPathHashCount(), hops);
      EXPECT_EQ(pkt.getPathHashSize(), sz);
    }
  }
}

TEST(Timekeeper, ReplayAndFreshnessAndForwardBound) {
  TestClock clk;
  const uint8_t none[1] = { 0x00 };
  const uint32_t fwd = NetTimeSync::MAX_FORWARD_SECS;

  // First advert after the hop-gate test: strictly newer than its last accept.
  uint32_t mark = kMinPl + 100;
  EXPECT_TRUE(Call(kTimekeeper, mark, none, 1, 0, clk));
  EXPECT_EQ(clk.t, mark);
  EXPECT_EQ(NetTimeSync::getHighWater(), mark);

  EXPECT_FALSE(Call(kTimekeeper, mark, none, 1, 0, clk));       // exact replay
  EXPECT_FALSE(Call(kTimekeeper, mark - 50, none, 1, 0, clk));  // stale
  EXPECT_EQ(clk.t, mark);

  EXPECT_TRUE(Call(kTimekeeper, mark + fwd, none, 1, 0, clk));  // boundary ok
  mark += fwd;
  EXPECT_FALSE(Call(kTimekeeper, mark + fwd + 1, none, 1, 0, clk));  // beyond
  EXPECT_EQ(NetTimeSync::getHighWater(), mark);

  // A clock that ran ahead is pulled back (corrections are bidirectional).
  clk.t = mark + 1000;
  EXPECT_TRUE(Call(kTimekeeper, mark + 5, none, 1, 0, clk));
  EXPECT_EQ(clk.t, mark + 5);
}

TEST(Timekeeper, AppliedDiffHasNoSignedOverflowPast2p31) {
  // Fix 7b746af7: the debug diff is computed on uint32 arithmetic. The local
  // clock straddles 2^31 while the timestamp is newer than every mark set by
  // the tests above; the clock must still be applied.
  TestClock clk;
  clk.t = 0x80000000u;
  const uint8_t none[1] = { 0x00 };
  EXPECT_TRUE(Call(kTimekeeper, 0x6C000000u, none, 1, 0, clk));
  EXPECT_EQ(clk.t, 0x6C000000u);
}

// RateLimiter (examples/simple_repeater/RateLimiter.h): the anon/discover
// limiters sit next to the timekeeper in the same clock domain, and a backward
// timekeeper correction used to pin their windows in the future (audit C-12).
TEST(RateLimiter, BackwardClockStepReanchorsWindow) {
  RateLimiter lim(4, 180);
  const uint32_t T = 1000000u;
  for (int i = 0; i < 4; i++) EXPECT_TRUE(lim.allow(T));
  EXPECT_FALSE(lim.allow(T + 10));                    // 5th within the window
  EXPECT_TRUE(lim.allow(T - 2700));                   // clock pulled back 45 min
  for (uint32_t i = 1; i < 4; i++) EXPECT_TRUE(lim.allow(T - 2700 + i));
  EXPECT_FALSE(lim.allow(T - 2700 + 10));             // 5th in the new window
  EXPECT_TRUE(lim.allow(T - 2700 + 181));             // new window expired
}

TEST(RateLimiter, ForwardFlowUnchanged) {
  RateLimiter lim(2, 120);
  EXPECT_TRUE(lim.allow(5000u));    // window [5000, 5120), count 1
  EXPECT_TRUE(lim.allow(5100u));    // count 2
  EXPECT_FALSE(lim.allow(5110u));   // count 3 > 2
  EXPECT_FALSE(lim.allow(5115u));   // still within, still denied
  EXPECT_TRUE(lim.allow(6000u));    // new window, count 1
  EXPECT_TRUE(lim.allow(6100u));    // count 2
  EXPECT_FALSE(lim.allow(6110u));   // denied
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
