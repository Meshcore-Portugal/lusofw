#pragma once
// Host shim: minimal Arduino-compatible surface for compiling lusofw feature
// sources off-device. Backed by an in-memory filesystem so RegionMap/Defaults
// persistence can be exercised for real.
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <string>
#include <vector>
#include <map>

#include "Stream.h"

#define PROGMEM
#define FILE_O_WRITE 1
#define FILE_O_READ  2

using std::toupper;

inline char* ltoa(long value, char* str, int base) {
  if (base == 16) {
    snprintf(str, 32, "%lx", value);
  } else if (base == 2) {
    char tmp[65];
    int i = 0;
    unsigned long v = (unsigned long)value;
    if (v == 0) tmp[i++] = '0';
    while (v) { tmp[i++] = '0' + (v & 1); v >>= 1; }
    int j = 0;
    while (i > 0) str[j++] = tmp[--i];
    str[j] = 0;
  } else {
    snprintf(str, 32, "%ld", value);
  }
  return str;
}

class File {
public:
  File() : _storage(nullptr), _pos(0), _wmode(false), _ok(false) {}
  static File makeWrite(std::vector<uint8_t>* s) {
    File f; f._storage = s; f._wmode = true; f._ok = true; return f;
  }
  static File makeRead(std::vector<uint8_t>* s) {
    File f; f._storage = s; f._wmode = false; f._ok = (s != nullptr); return f;
  }
  operator bool() const { return _ok && _storage != nullptr; }
  int read() {
    if (!_ok || _wmode || !_storage || _pos >= _storage->size()) return -1;
    return (*_storage)[_pos++];
  }
  size_t read(uint8_t* buf, size_t n) {
    size_t got = 0;
    while (got < n) { int c = read(); if (c < 0) break; buf[got++] = (uint8_t)c; }
    return got;
  }
  size_t readBytes(char* buf, size_t n) { return read((uint8_t*)buf, n); }
  size_t write(const uint8_t* buf, size_t n) {
    if (!_ok || !_wmode || !_storage) return 0;
    _storage->insert(_storage->end(), buf, buf + n);
    return n;
  }
  size_t write(uint8_t c) { return write(&c, 1); }
  size_t print(const char* s) { return write((const uint8_t*)s, strlen(s)); }
  size_t available() { return (_ok && !_wmode && _storage) ? (_storage->size() - _pos) : 0; }
  void close() { _ok = false; }
  void flush() {}
private:
  std::vector<uint8_t>* _storage;
  size_t _pos;
  bool _wmode;
  bool _ok;
};

class FILESYSTEM {
public:
  bool exists(const char* path) const { return _files.count(path) != 0; }
  File open(const char* path, int mode = FILE_O_READ) {
    if (mode == FILE_O_WRITE) {
      auto it = _files.find(path);
      if (it == _files.end()) it = _files.emplace(path, std::vector<uint8_t>()).first;
      it->second.clear();
      writes++;
      return File::makeWrite(&it->second);
    }
    reads++;
    auto it = _files.find(path);
    if (it == _files.end()) return File();
    return File::makeRead(&it->second);
  }
  File open(const char* path, const char* mode, bool = true) {
    return open(path, mode && mode[0] == 'w' ? (int)FILE_O_WRITE : (int)FILE_O_READ);
  }
  bool remove(const char* path) { return _files.erase(path) > 0; }
  const std::vector<uint8_t>* bytes(const char* path) const {
    auto it = _files.find(path);
    return it == _files.end() ? nullptr : &it->second;
  }
  size_t writes = 0, reads = 0;
private:
  std::map<std::string, std::vector<uint8_t>> _files;
};
