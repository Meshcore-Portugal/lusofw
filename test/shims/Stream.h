#pragma once
// Host shim for Arduino Stream (RegionMap's BufStream derives from it).
#include <cstdio>
#include <cstdarg>
#include <cstring>

class Stream {
public:
  virtual ~Stream() {}
  virtual size_t write(uint8_t) = 0;
  virtual size_t write(const uint8_t*, size_t) = 0;
  virtual int available() { return 0; }
  virtual int read() { return -1; }
  virtual int peek() { return -1; }
  virtual void flush() {}
  size_t print(const char* s) { return write((const uint8_t*)s, strlen(s)); }
  size_t print(char c) { return write((uint8_t)c); }
  size_t println(const char* s) { size_t n = print(s); n += write((uint8_t)'\n'); return n; }
  void printf(const char* fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    print(buf);
  }
};
