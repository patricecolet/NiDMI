#pragma once
// Minimal Arduino stub for native unit tests (g++ on macOS/Linux)
// Only provides types and functions used by testable NiDMI code.

#include <cstdint>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <string>
#include <algorithm>

typedef uint8_t byte;

// Arduino map() function
inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
    if (in_max == in_min) return out_min;
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Arduino constrain() function
template<typename T>
inline T constrain(T x, T lo, T hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

// Minimal Arduino String class (subset used by JSONParser)
class String {
    std::string _buf;
public:
    String() {}
    String(const char* s) : _buf(s ? s : "") {}
    String(const std::string& s) : _buf(s) {}
    String(char c) : _buf(1, c) {}

    String operator+(const String& rhs) const { return String(_buf + rhs._buf); }
    String operator+(const char* rhs) const { return String(_buf + (rhs ? rhs : "")); }
    String& operator+=(char c) { _buf += c; return *this; }
    String& operator+=(const char* s) { if (s) _buf += s; return *this; }
    bool operator==(const String& rhs) const { return _buf == rhs._buf; }
    bool operator==(const char* rhs) const { return _buf == (rhs ? rhs : ""); }
    char operator[](unsigned int i) const { return _buf[i]; }

    unsigned int length() const { return _buf.size(); }
    const char* c_str() const { return _buf.c_str(); }

    int indexOf(const String& sub) const {
        auto pos = _buf.find(sub._buf);
        return pos == std::string::npos ? -1 : (int)pos;
    }
    int indexOf(const String& sub, unsigned int from) const {
        auto pos = _buf.find(sub._buf, from);
        return pos == std::string::npos ? -1 : (int)pos;
    }

    String substring(unsigned int from, unsigned int to) const {
        if (from >= _buf.size()) return String();
        if (to > _buf.size()) to = _buf.size();
        return String(_buf.substr(from, to - from));
    }

    int toInt() const { return std::atoi(_buf.c_str()); }

    bool startsWith(const char* prefix, unsigned int offset = 0) const {
        if (!prefix) return false;
        size_t len = strlen(prefix);
        if (offset + len > _buf.size()) return false;
        return _buf.compare(offset, len, prefix) == 0;
    }

    void reserve(unsigned int size) { _buf.reserve(size); }
    void trim() {
        auto start = _buf.find_first_not_of(" \t\r\n");
        auto end = _buf.find_last_not_of(" \t\r\n");
        if (start == std::string::npos) { _buf.clear(); return; }
        _buf = _buf.substr(start, end - start + 1);
    }
    void replace(const char* from, const char* to) {
        std::string f(from), t(to);
        size_t pos = 0;
        while ((pos = _buf.find(f, pos)) != std::string::npos) {
            _buf.replace(pos, f.size(), t);
            pos += t.size();
        }
    }
};

// isdigit is provided by <cctype> (included transitively) — no need to redefine
