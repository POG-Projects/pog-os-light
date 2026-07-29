#include "config.h"
#include <Preferences.h>

Config            g_config;
SemaphoreHandle_t g_configMutex = nullptr;

static Preferences prefs;
static const char* NVS_NS  = "poglight";
static const char* NVS_KEY = "cfg";

static const char* PURPOSE_KEYS[LP_COUNT] = {
  "ambient", "task", "night", "path", "tv", "status", "wake", "decorative"
};

static const char* PURPOSE_LABELS[LP_COUNT] = {
  "Ambiance", "Éclairage de travail", "Veilleuse", "Balisage",
  "Télévision", "Indicateur d’état", "Réveil", "Décoration"
};

const char* lightPurposeKey(uint8_t purpose) {
  return PURPOSE_KEYS[purpose < LP_COUNT ? purpose : LP_AMBIENT];
}

const char* lightPurposeLabel(uint8_t purpose) {
  return PURPOSE_LABELS[purpose < LP_COUNT ? purpose : LP_AMBIENT];
}

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
  root["oledEnabled"] = g_config.oledEnabled;
  root["oledSda"] = g_config.oledSda;
  root["oledScl"] = g_config.oledScl;
  root["oledAddress"] = g_config.oledAddress;
  root["buttonsEnabled"] = g_config.buttonsEnabled;
  root["buttonMode"] = g_config.buttonMode;
  JsonArray buttonPins = root["buttonPins"].to<JsonArray>();
  for (uint8_t pin : g_config.buttonPins) buttonPins.add(pin);
  root["pattern"]    = g_config.pattern;
  root["primaryColor"]   = g_config.primaryColor;
  root["secondaryColor"] = g_config.secondaryColor;
  root["speed"]      = g_config.speed;
  root["purpose"]    = g_config.purpose;
  root["nextSectionId"] = g_config.nextSectionId;
  JsonArray sections = root["sections"].to<JsonArray>();
  for (uint8_t i = 0; i < g_config.sectionCount; ++i) {
    const LedSection& section = g_config.sections[i];
    JsonObject out = sections.add<JsonObject>();
    out["id"] = section.id;
    out["name"] = section.name;
    out["start"] = section.start;
    out["count"] = section.count;
    out["enabled"] = section.enabled;
    out["on"] = section.on;
    out["purpose"] = section.purpose;
    out["purposeKey"] = lightPurposeKey(section.purpose);
    out["brightness"] = section.brightness;
    out["pattern"] = section.pattern;
    out["primaryColor"] = section.primaryColor;
    out["secondaryColor"] = section.secondaryColor;
    out["speed"] = section.speed;
  }
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
  if (!doc["oledEnabled"].isNull()) g_config.oledEnabled = doc["oledEnabled"].as<bool>();
  bool explicitOledPins = !doc["oledSda"].isNull() || !doc["oledScl"].isNull();
  if (!doc["oledSda"].isNull()) g_config.oledSda = (uint8_t) constrain(doc["oledSda"].as<int>(), 0, 48);
  if (!doc["oledScl"].isNull()) g_config.oledScl = (uint8_t) constrain(doc["oledScl"].as<int>(), 0, 48);
  if (!doc["oledAddress"].isNull()) {
    g_config.oledAddress = (uint8_t) constrain(doc["oledAddress"].as<int>(), 0x08, 0x77);
  }
  if (!explicitOledPins && !doc["oledSwap"].isNull()) {
#if CONFIG_IDF_TARGET_ESP32C3
    // Les anciennes versions C3 etaient toujours headless : 11/13 ne sont pas
    // des broches exposees sur le SuperMini, on migre vers une paire utilisable.
    g_config.oledSda = 5;
    g_config.oledScl = 6;
#else
    bool swapped = doc["oledSwap"].as<bool>();
    g_config.oledSda = swapped ? OLED_SCL_PIN : OLED_SDA_PIN;
    g_config.oledScl = swapped ? OLED_SDA_PIN : OLED_SCL_PIN;
#endif
  }
#if CONFIG_IDF_TARGET_ESP32C3
  if (!g_config.oledEnabled && g_config.oledSda == 11 && g_config.oledScl == 13) {
    g_config.oledSda = 5;
    g_config.oledScl = 6;
  }
#endif
  g_config.oledSwap = g_config.oledSda == OLED_SCL_PIN && g_config.oledScl == OLED_SDA_PIN;
  if (!doc["buttonsEnabled"].isNull()) g_config.buttonsEnabled = doc["buttonsEnabled"].as<bool>();
  if (!doc["buttonMode"].isNull()) {
    g_config.buttonMode = (uint8_t) constrain(doc["buttonMode"].as<int>(), 0, BIM_COUNT - 1);
  }
  if (doc["buttonPins"].is<JsonArrayConst>()) {
    uint8_t i = 0;
    for (JsonVariantConst pin : doc["buttonPins"].as<JsonArrayConst>()) {
      if (i >= 4) break;
      g_config.buttonPins[i++] = (uint8_t) constrain(pin.as<int>(), 0, 48);
    }
  }
  if (!doc["pattern"].isNull())    g_config.pattern    = (uint8_t) constrain(doc["pattern"].as<int>(), 0, TP_COUNT - 1);
  if (!doc["primaryColor"].isNull())   g_config.primaryColor   = doc["primaryColor"].as<uint32_t>() & 0xFFFFFF;
  if (!doc["secondaryColor"].isNull()) g_config.secondaryColor = doc["secondaryColor"].as<uint32_t>() & 0xFFFFFF;
  if (!doc["speed"].isNull())      g_config.speed      = (uint8_t) constrain(doc["speed"].as<int>(), 0, 100);
  if (!doc["purpose"].isNull())    g_config.purpose    = (uint8_t) constrain(doc["purpose"].as<int>(), 0, LP_COUNT - 1);
  if (!doc["nextSectionId"].isNull()) {
    g_config.nextSectionId = max((uint16_t)1, doc["nextSectionId"].as<uint16_t>());
  }
  if (doc["sections"].is<JsonArrayConst>()) {
    g_config.sectionCount = 0;
    uint16_t highestId = 0;
    for (JsonObjectConst item : doc["sections"].as<JsonArrayConst>()) {
      if (g_config.sectionCount >= MAX_SECTIONS) break;
      LedSection section;
      if (item["id"].isNull()) section.id = g_config.nextSectionId++;
      else section.id = item["id"].as<uint16_t>();
      if (!section.id) section.id = g_config.nextSectionId++;
      highestId = max(highestId, section.id);
      if (item["name"].is<const char*>()) {
        section.name = item["name"].as<String>();
        section.name.trim();
      }
      if (!section.name.length()) section.name = "Section " + String(g_config.sectionCount + 1);
      section.start = (uint16_t) constrain(item["start"] | 0, 0, max(0, (int)g_config.numLeds - 1));
      section.count = (uint16_t) constrain(item["count"] | 1, 1, (int)g_config.numLeds - section.start);
      section.enabled = item["enabled"] | true;
      section.on = item["on"] | true;
      section.purpose = (uint8_t) constrain(item["purpose"] | (int)LP_AMBIENT, 0, LP_COUNT - 1);
      section.brightness = (uint8_t) constrain(item["brightness"] | 255, 0, 255);
      section.pattern = (uint8_t) constrain(item["pattern"] | (int)TP_SOLID, 0, TP_COUNT - 1);
      section.primaryColor = (item["primaryColor"] | 0x19C37D) & 0xFFFFFF;
      section.secondaryColor = (item["secondaryColor"] | 0x6C3BFF) & 0xFFFFFF;
      section.speed = (uint8_t) constrain(item["speed"] | 50, 0, 100);
      g_config.sections[g_config.sectionCount++] = section;
    }
    g_config.nextSectionId = max(g_config.nextSectionId, (uint16_t)(highestId + 1));
  }
  // Les réglages de la scène principale servent de commande de groupe lorsque
  // le ruban est découpé. Un payload de section n'inclut pas ces champs.
  for (uint8_t i = 0; i < g_config.sectionCount; ++i) {
    if (!doc["pattern"].isNull()) g_config.sections[i].pattern = g_config.pattern;
    if (!doc["primaryColor"].isNull()) g_config.sections[i].primaryColor = g_config.primaryColor;
    if (!doc["secondaryColor"].isNull()) g_config.sections[i].secondaryColor = g_config.secondaryColor;
    if (!doc["speed"].isNull()) g_config.sections[i].speed = g_config.speed;
  }
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
