#pragma once

#include <stdint.h>
#include <string.h>

// Sliding 1 h airtime window in 1-min slots (LUSOFW_DUTY_WINDOW).
// Covers the current partial slot plus 60 full ones (60..61 min), so expiry is
// never lenient: airtime in ANY 1 h window stays <= the limit checked by the caller.
// All time math is unsigned deltas, so millis() wrap is safe.
class DutyCycleWindow {
  static const int SLOTS = 61;
  uint32_t slot_ms[SLOTS];
  uint32_t used_ms;
  uint32_t slot_start;
  int cur;

public:
  static const uint32_t SLOT_MS = 60000;

  void reset(uint32_t now) {
    memset(slot_ms, 0, sizeof(slot_ms));
    used_ms = 0;
    slot_start = now;
    cur = 0;
  }

  void advance(uint32_t now) {
    if (now - slot_start >= SLOTS * SLOT_MS) { reset(now); return; }  // idle > window: all expired
    while (now - slot_start >= SLOT_MS) {
      slot_start += SLOT_MS;
      cur = (cur + 1) % SLOTS;
      used_ms -= slot_ms[cur];
      slot_ms[cur] = 0;
    }
  }

  void add(uint32_t now, uint32_t ms) { advance(now); slot_ms[cur] += ms; used_ms += ms; }
  uint32_t used(uint32_t now) { advance(now); return used_ms; }
  uint32_t msToNextSlot(uint32_t now) { advance(now); return SLOT_MS - (now - slot_start); }
};
