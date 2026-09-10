#pragma once
// Host shim for RTClib.h: only DateTime(uint32_t) + civil accessors used by
// NetTimeSync's debug formatting (compiled in even with debug output off).
#include <stdint.h>

class DateTime {
public:
  explicit DateTime(uint32_t t) : _t(t) {}
  uint16_t year() const { return (uint16_t)civil_from(_t / 86400u).y; }
  uint8_t month() const { return (uint8_t)civil_from(_t / 86400u).m; }
  uint8_t day() const { return (uint8_t)civil_from(_t / 86400u).d; }
  uint8_t hour() const { return (uint8_t)((_t / 3600u) % 24u); }
  uint8_t minute() const { return (uint8_t)((_t / 60u) % 60u); }
  uint8_t second() const { return (uint8_t)(_t % 60u); }
private:
  struct YMD { long y; unsigned m, d; };
  static YMD civil_from(uint32_t days) {
    long z = (long)days + 719468;
    long era = (z >= 0 ? z : z - 146096) / 146097;
    long doe = z - era * 146097;
    long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    long y = yoe + era * 400;
    long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    long mp = (5 * doy + 2) / 153;
    long d = doy - (153 * mp + 2) / 5 + 1;
    unsigned m = (unsigned)(mp + (mp < 10 ? 3 : -9));
    YMD r;
    r.y = y + (m <= 2 ? 1 : 0);
    r.m = m;
    r.d = (unsigned)d;
    return r;
  }
  uint32_t _t;
};
