#include <gtest/gtest.h>
#include <vector>
#include "lusofw/DutyCycleWindow.h"

static const uint32_t MIN = DutyCycleWindow::SLOT_MS;

TEST(DutyCycleWindow, AccumulatesWithinWindow) {
  DutyCycleWindow w; w.reset(0);
  w.add(0, 1000);
  w.add(30 * MIN, 2000);
  EXPECT_EQ(3000u, w.used(59 * MIN));
}

TEST(DutyCycleWindow, NeverExpiresBeforeOneHour) {
  DutyCycleWindow w; w.reset(0);
  w.add(MIN - 1, 500);                        // end of first slot
  EXPECT_EQ(500u, w.used(MIN - 1 + 60 * MIN - 1));  // 1 ms short of 1 h later: still counted
  EXPECT_EQ(0u, w.used(62 * MIN));            // gone after window + guard slot
}

TEST(DutyCycleWindow, LongIdleClearsAll) {
  DutyCycleWindow w; w.reset(0);
  w.add(0, 9999);
  EXPECT_EQ(0u, w.used(5 * 60 * MIN));
}

TEST(DutyCycleWindow, MillisWrap) {
  uint32_t t0 = 0xFFFFFFFFu - 10 * MIN;
  DutyCycleWindow w; w.reset(t0);
  w.add(t0, 700);
  EXPECT_EQ(700u, w.used(t0 + 30 * MIN));     // wraps past 0
  EXPECT_EQ(0u, w.used(t0 + 62 * MIN));
}

TEST(DutyCycleWindow, NextSlotWait) {
  DutyCycleWindow w; w.reset(0);
  EXPECT_EQ(MIN - 15000, w.msToNextSlot(15000));
  EXPECT_EQ(MIN, w.msToNextSlot(MIN));
}

// Sustained saturation with the Dispatcher gate rule: no 1 h window may exceed the limit.
TEST(DutyCycleWindow, SaturatedSenderNeverExceedsLimit) {
  const uint32_t limit = 360000, pkt = 1510;   // 10%, max-size SF8/BW62.5 packet
  DutyCycleWindow w; w.reset(0);
  std::vector<std::pair<uint32_t, uint32_t>> tx;
  uint32_t now = 0;
  while (now < 5 * 60 * MIN) {
    if (w.used(now) + pkt <= limit) { w.add(now, pkt); tx.push_back({now, pkt}); now += pkt; }
    else now += w.msToNextSlot(now);
  }
  for (size_t i = 0; i < tx.size(); i++) {
    uint64_t sum = 0;
    for (size_t j = i; j < tx.size() && tx[j].first < tx[i].first + 60 * MIN; j++) sum += tx[j].second;
    ASSERT_LE(sum, limit) << "window starting at " << tx[i].first;
  }
  EXPECT_GT(tx.size() * pkt, 4 * limit);       // still uses most of the allowance
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
