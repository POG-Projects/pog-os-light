// Substitut d'Arduino.h pour la compilation sur l'hôte.
//
// Il n'existe que pour compiler src/config.cpp hors de l'ESP32 et vérifier
// hardwarePinsValid par un test automatique. Il ne cherche pas à imiter
// Arduino : juste à fournir ce que config.cpp utilise réellement. Toute
// tentative d'en faire un vrai socle serait un second firmware à entretenir.
#pragma once

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

class String {
 public:
  String() {}
  String(const char *s) : v_(s ? s : "") {}
  String(const std::string &s) : v_(s) {}
  explicit String(int n) { char b[16]; snprintf(b, sizeof(b), "%d", n); v_ = b; }
  explicit String(unsigned n) { char b[16]; snprintf(b, sizeof(b), "%u", n); v_ = b; }

  String &operator=(const char *s) { v_ = s ? s : ""; return *this; }

  const char *c_str() const { return v_.c_str(); }
  unsigned length() const { return (unsigned)v_.size(); }
  bool concat(const char *s) { if (s) v_ += s; return true; }
  void trim() {
    size_t a = v_.find_first_not_of(" \t\r\n");
    size_t b = v_.find_last_not_of(" \t\r\n");
    v_ = (a == std::string::npos) ? std::string() : v_.substr(a, b - a + 1);
  }
  String &operator+=(const String &o) { v_ += o.v_; return *this; }
  String &operator+=(const char *s) { if (s) v_ += s; return *this; }
  friend String operator+(String a, const String &b) { a += b; return a; }
  friend String operator+(String a, const char *b) { a += b; return a; }
  friend String operator+(const char *a, const String &b) { return String(a) + b; }
  bool operator==(const String &o) const { return v_ == o.v_; }
  bool operator==(const char *s) const { return v_ == (s ? s : ""); }
  bool operator!=(const String &o) const { return !(*this == o); }

 private:
  std::string v_;
};

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef constrain
#define constrain(x, lo, hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))
#endif
