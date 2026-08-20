// Substitut de Preferences (NVS) pour l'hôte : le test ne persiste rien.
#pragma once
#include <Arduino.h>

class Preferences {
 public:
  bool begin(const char *, bool = false) { return true; }
  void end() {}
  String getString(const char *, const char *fallback = "") { return String(fallback); }
  size_t putString(const char *, const String &v) { return v.length(); }
  void clear() {}
};
