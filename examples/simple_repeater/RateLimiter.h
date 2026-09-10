#pragma once

#include <stdint.h>

class RateLimiter {
  uint32_t _start_timestamp;
  uint32_t _secs;
  uint16_t _maximum, _count;

public:
  RateLimiter(uint16_t maximum, uint32_t secs): _maximum(maximum), _secs(secs), _start_timestamp(0), _count(0) { }

  bool allow(uint32_t now) {
    if (now < _start_timestamp) {
      // Clock stepped backwards (eg. a timekeeper correction pulled the RTC
      // back): the old window now ends in the future and would pin the
      // limiter shut for the whole correction delta. Re-anchor the window at
      // 'now' and start counting again.
      _start_timestamp = now;
      _count = 1;
      return true;
    }
    if (now < _start_timestamp + _secs) {
      _count++;
      if (_count > _maximum) return false;   // deny
    } else {   // time window now expired
      _start_timestamp = now;
      _count = 1;
    }
    return true;
  }
};