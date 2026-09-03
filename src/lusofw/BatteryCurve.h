#pragma once

#include <stdint.h>

// Per-cell full/empty voltages used by the curve below; boards with different
// chemistries or cell counts override them via build flags (2S: 8400/6600).
#ifndef BATT_MAX_MILLIVOLTS
  #define BATT_MAX_MILLIVOLTS 4200
#endif

/*
 * LiPo state-of-charge estimation shared by the lusofw UIs (simple_repeater,
 * companion_radio ui-new and ui-orig).
 *
 * Fully static: one pure function, no state. The number of cells in series is
 * derived from BATT_MAX_MILLIVOLTS, so the same per-cell curve works for
 * 1S (4.2 V) and 2S (8.4 V) packs.
 */
class BatteryCurve {
public:
  // LiPo discharge curve: per-cell open-circuit voltage (mV) -> remaining
  // capacity (%). Approximates a standard 1S LiPo at a low discharge rate;
  // linearly interpolated between points. The non-linear S-shape (steep at
  // the top, a long flat plateau through the mid-range, steep drop near
  // empty) tracks real cell behaviour far better than a straight
  // (v - min) / (max - min) line.
  struct Point {
    uint16_t milliVolts;
    uint8_t percent;
  };
  static inline const Point CURVE[15] = {
    { 4200, 100 }, { 4150, 95 }, { 4100, 90 }, { 4050, 85 }, { 4000, 80 },
    { 3950, 74 }, { 3900, 68 }, { 3850, 60 }, { 3800, 50 }, { 3750, 38 },
    { 3700, 28 }, { 3650, 18 }, { 3600, 10 }, { 3500,  5 }, { 3300,  0 },
  };

  /**
   * \brief  Map a battery voltage to a state-of-charge percentage (clamped to
   *         [0, 100]) by interpolating the discharge curve above.
   */
  static int lipoPercentFromMilliVolts(uint16_t batteryMilliVolts) {
    const int numCells = (BATT_MAX_MILLIVOLTS + 2099) / 4200;   // round(4.2 V per cell)
    const uint16_t cellMilliVolts = batteryMilliVolts / numCells;

    const int n = sizeof(CURVE) / sizeof(CURVE[0]);
    if (cellMilliVolts >= CURVE[0].milliVolts) return 100;
    if (cellMilliVolts <= CURVE[n - 1].milliVolts) return 0;

    for (int i = 0; i < n - 1; i++) {
      const uint16_t vHi = CURVE[i].milliVolts;
      const uint16_t vLo = CURVE[i + 1].milliVolts;
      if (cellMilliVolts <= vHi && cellMilliVolts >= vLo) {
        const int pHi = CURVE[i].percent;
        const int pLo = CURVE[i + 1].percent;
        return pLo + (int)(cellMilliVolts - vLo) * (pHi - pLo) / (int)(vHi - vLo);
      }
    }
    return 0;
  }
};
