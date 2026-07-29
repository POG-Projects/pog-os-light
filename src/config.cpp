#include "config.h"
#include <Preferences.h>

Config            g_config;
SemaphoreHandle_t g_configMutex = nullptr;

static Preferences prefs;
static const char* NVS_NS  = "poglight";
static const char* NVS_KEY = "cfg";

void configBegin() {
  if (!g_configMutex) g_configMutex = xSemaphoreCreateMutex();
}

void configFillJson(JsonObject root, bool includeWifiPass) {
  root["ledPin"]     = g_config.ledPin;
  root["numLeds"]    = g_config.numLeds;
  root["brightness"] = g_config.brightness;
  root["maxMilliAmps"] = g_config.maxMilliAmps;
  root["colorOrder"] = g_config.colorOrder;
  root["analog"]     = g_config.analog;
  root["reverse"]    = g_config.reverse;
  root["oledSwap"]   = g_config.oledSwap;
  root["pattern"]    = g_config.pattern;
  root["primaryColor"]   = g_config.primaryColor;
  root["secondaryColor"] = g_config.secondaryColor;
  root["speed"]      = g_config.speed;
  root["wifiSsid"]   = g_config.wifiSsid;
  if (includeWifiPass) root["wifiPass"] = g_config.wifiPass;
}

void configApplyJson(JsonObjectConst doc) {
  if (!doc["ledPin"].isNull())     g_config.ledPin     = doc["ledPin"].as<uint8_t>();
  if (!doc["numLeds"].isNull())    g_config.numLeds    = (uint16_t) constrain(doc["numLeds"].as<int>(), 1, MAX_LEDS);
  if (!doc["brightness"].isNull()) g_config.brightness = doc["brightness"].as<uint8_t>();
  if (!doc["maxMilliAmps"].isNull()) g_config.maxMilliAmps = (uint16_t) constrain(doc["maxMilliAmps"].as<int>(), 100, 10000);
  if (!doc["colorOrder"].isNull()) g_config.colorOrder = (uint8_t) constrain(doc["colorOrder"].as<int>(), 0, ORDER_COUNT - 1);
  if (!doc["analog"].isNull())     g_config.analog     = doc["analog"].as<bool>();
  if (!doc["reverse"].isNull())    g_config.reverse    = doc["reverse"].as<bool>();
  if (!doc["oledSwap"].isNull())   g_config.oledSwap   = doc["oledSwap"].as<bool>();
  if (!doc["pattern"].isNull())    g_config.pattern    = (uint8_t) constrain(doc["pattern"].as<int>(), 0, TP_COUNT - 1);
  if (!doc["primaryColor"].isNull())   g_config.primaryColor   = doc["primaryColor"].as<uint32_t>() & 0xFFFFFF;
  if (!doc["secondaryColor"].isNull()) g_config.secondaryColor = doc["secondaryColor"].as<uint32_t>() & 0xFFFFFF;
  if (!doc["speed"].isNull())      g_config.speed      = (uint8_t) constrain(doc["speed"].as<int>(), 0, 100);
  if (doc["wifiSsid"].is<const char*>()) g_config.wifiSsid = doc["wifiSsid"].as<String>();
  if (doc["wifiPass"].is<const char*>()) { String p = doc["wifiPass"].as<String>(); if (p.length()) g_config.wifiPass = p; }
}

bool configLoad() {
  prefs.begin(NVS_NS, true);
  String json = prefs.getString(NVS_KEY, "");
  prefs.end();
  if (json.length() == 0) return false;
  JsonDocument doc;
  if (deserializeJson(doc, json)) return false;
  configApplyJson(doc.as<JsonObjectConst>());
  return true;
}

void configSave() {
  JsonDocument doc;
  configFillJson(doc.to<JsonObject>(), true);
  String json;
  serializeJson(doc, json);
  prefs.begin(NVS_NS, false);
  prefs.putString(NVS_KEY, json);
  prefs.end();
}
