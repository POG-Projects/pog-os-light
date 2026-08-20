#include "pogdev.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <esp_mac.h>
#include <esp_system.h>

#include <FastLED.h>

#include "config.h"

#ifndef POGLIGHT_FW_VERSION
#define POGLIGHT_FW_VERSION "dev"
#endif

namespace {

constexpr char kNamespace[] = "pogdev";
constexpr uint32_t kStatePeriodMs = 30000;
constexpr uint32_t kReconnectPeriodMs = 5000;
// Palier de reprise après une publication refusée : on double jusqu’au plafond.
constexpr uint32_t kMinBackoffMs = 500;
constexpr uint32_t kMaxBackoffMs = 30000;

struct Credentials {
  String deviceId;
  String host;
  uint16_t port = 1883;
  String password;

  bool valid() const {
    return deviceId.length() && host.length() && password.length();
  }
};

WiFiClient mqttTransport;
PubSubClient mqtt(mqttTransport);
Credentials credentials;
String claimSecret;
String hardwareId;
IPAddress poghomeAddress;
uint16_t poghomeApiPort = 8090;
TaskHandle_t taskHandle = nullptr;
volatile bool stateDirty = true;
volatile bool helloDirty = true;
volatile uint32_t rebootAt = 0;
uint8_t lastActivePattern = TP_RAINBOW;

const char *kEffectNames[TP_COUNT] = {
    "Uni", "Ordre couleurs", "Pixel mobile", "Remplissage", "Arc-en-ciel",
    "Chenillard", "Respiration", "Feu", "Scintillement", "Dégradé",
    "Balayage", "Blanc", "Éteint", "Aurore", "Océan", "Lave",
    "Comète", "Vagues", "Bougie"};
const char *kColorOrders[ORDER_COUNT] = {"RGB", "RBG", "GRB", "GBR", "BRG", "BGR"};

JsonObject addEntity(JsonArray entities, const String &key, const String &name,
                     const char *category) {
  JsonObject entity = entities.add<JsonObject>();
  entity["key"] = key;
  entity["name"] = name;
  entity["category"] = category;
  return entity;
}

void addNumberEntity(JsonArray entities, const String &key, const String &name,
                     const char *category, float minValue, float maxValue,
                     float step = 1, const char *unit = nullptr) {
  JsonObject entity = addEntity(entities, key, name, category);
  JsonObject trait = entity["traits"].to<JsonArray>().add<JsonObject>();
  trait["id"] = "number";
  trait["config"]["min"] = minValue;
  trait["config"]["max"] = maxValue;
  trait["config"]["step"] = step;
  if (unit) trait["config"]["unit"] = unit;
}

void addSelectEntity(JsonArray entities, const String &key, const String &name,
                     const char *category, const char *const *values, size_t count) {
  JsonObject entity = addEntity(entities, key, name, category);
  JsonObject trait = entity["traits"].to<JsonArray>().add<JsonObject>();
  trait["id"] = "select";
  JsonArray options = trait["config"]["options"].to<JsonArray>();
  for (size_t i = 0; i < count; ++i) options.add(values[i]);
}

void addSwitchEntity(JsonArray entities, const String &key, const String &name,
                     const char *category) {
  JsonObject entity = addEntity(entities, key, name, category);
  entity["traits"].to<JsonArray>().add<JsonObject>()["id"] = "on_off";
}

void addTextEntity(JsonArray entities, const String &key, const String &name,
                   const char *category, bool password = false) {
  JsonObject entity = addEntity(entities, key, name, category);
  JsonObject trait = entity["traits"].to<JsonArray>().add<JsonObject>();
  trait["id"] = "text";
  trait["config"]["max_length"] = 64;
  if (password) trait["config"]["password"] = true;
}

void addColorEntity(JsonArray entities, const String &key, const String &name,
                    const char *category) {
  JsonObject entity = addEntity(entities, key, name, category);
  entity["traits"].to<JsonArray>().add<JsonObject>()["id"] = "color";
}

// Synchronisation d'ambiance et choix d'utilité : deux gestes de la lampe, pas
// deux objets du foyer. Ils étaient déclarés comme les entités « room_sync » et
// « purpose » ; ce sont désormais deux commandes du trait `action` porté par
// l'entité meneuse « light ». Le trait `action` a le domaine "switch" côté
// PogHome, donc il ne déloge pas le domaine "light" : le slug reste le même.
void addLightActions(JsonArray traits, const char *const *purposeLabels) {
  JsonObject trait = traits.add<JsonObject>();
  trait["id"] = "action";
  JsonArray commands = trait["config"]["commands"].to<JsonArray>();

  JsonObject sync = commands.add<JsonObject>();
  sync["name"] = "sync_effect";
  sync["label"] = "Synchroniser l’ambiance";
  sync["sensitive"] = false;
  sync["reversible"] = true;
  JsonArray syncParams = sync["params"].to<JsonArray>();

  JsonObject effect = syncParams.add<JsonObject>();
  effect["name"] = "effect";
  effect["kind"] = "enum";
  effect["required"] = true;
  JsonArray options = effect["enum"].to<JsonArray>();
  for (const char *name : kEffectNames) options.add(name);

  auto numberParam = [&](const char *name, float min, float max) {
    JsonObject param = syncParams.add<JsonObject>();
    param["name"] = name;
    param["kind"] = "number";
    param["required"] = true;
    param["min"] = min;
    param["max"] = max;
  };
  numberParam("speed", 0, 100);
  numberParam("brightness", 0, 100);
  numberParam("primary_hue", 0, 360);
  numberParam("primary_saturation", 0, 100);
  numberParam("secondary_hue", 0, 360);
  numberParam("secondary_saturation", 0, 100);

  JsonObject setPurpose = commands.add<JsonObject>();
  setPurpose["name"] = "set_purpose";
  setPurpose["label"] = "Choisir l’utilité";
  setPurpose["sensitive"] = false;
  setPurpose["reversible"] = true;
  JsonObject purposeParam =
      setPurpose["params"].to<JsonArray>().add<JsonObject>();
  purposeParam["name"] = "purpose";
  purposeParam["kind"] = "enum";
  purposeParam["required"] = true;
  JsonArray purposeOptions = purposeParam["enum"].to<JsonArray>();
  for (uint8_t i = 0; i < LP_COUNT; ++i) purposeOptions.add(purposeLabels[i]);
}

int purposeFromLabel(const String &option) {
  for (uint8_t i = 0; i < LP_COUNT; ++i) {
    if (option == lightPurposeLabel(i)) return i;
  }
  return -1;
}

String makeHardwareId() {
  uint8_t mac[6] = {};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char out[32];
  snprintf(out, sizeof(out), "ESP-POGLIGHT-%02X%02X%02X%02X%02X%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(out);
}

String makeSecret() {
  char out[65];
  for (size_t i = 0; i < 32; ++i) {
    uint8_t value = esp_random() & 0xff;
    snprintf(out + i * 2, 3, "%02x", value);
  }
  out[64] = '\0';
  return String(out);
}

void loadIdentity() {
  Preferences prefs;
  prefs.begin(kNamespace, true);
  claimSecret = prefs.getString("claim", "");
  credentials.deviceId = prefs.getString("dev_id", "");
  credentials.host = prefs.getString("host", "");
  credentials.port = prefs.getUShort("port", 1883);
  credentials.password = prefs.getString("password", "");
  prefs.end();

  if (!claimSecret.length()) {
    claimSecret = makeSecret();
    prefs.begin(kNamespace, false);
    prefs.putString("claim", claimSecret);
    prefs.end();
  }
}

bool saveCredentials(const Credentials &next) {
  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) return false;
  bool ok = prefs.putString("dev_id", next.deviceId) &&
            prefs.putString("host", next.host) &&
            prefs.putUShort("port", next.port) &&
            prefs.putString("password", next.password);
  prefs.end();
  if (ok) credentials = next;
  return ok;
}

void clearIdentity() {
  mqtt.disconnect();
  Preferences prefs;
  prefs.begin(kNamespace, false);
  prefs.clear();
  prefs.end();
  credentials = Credentials{};
  claimSecret = makeSecret();
  prefs.begin(kNamespace, false);
  prefs.putString("claim", claimSecret);
  prefs.end();
}

bool discoverPogHome() {
  int count = MDNS.queryService("poghome", "tcp");
  for (int i = 0; i < count; ++i) {
    String proto = MDNS.txt(i, "proto");
    if (proto.length() && proto != "1") continue;
    poghomeAddress = MDNS.address(i);
    if (!poghomeAddress) continue;
    String api = MDNS.txt(i, "api");
    poghomeApiPort = api.length() ? api.toInt() : MDNS.port(i);
    if (!poghomeApiPort) poghomeApiPort = 8090;
    Serial.printf("[PogHome] détecté sur %s:%u\n",
                  poghomeAddress.toString().c_str(), poghomeApiPort);
    return true;
  }
  return false;
}

String apiUrl(const String &path) {
  return String("http://") + poghomeAddress.toString() + ":" +
         String(poghomeApiPort) + path;
}

int httpPostJson(const String &path, const String &body, String &response) {
  WiFiClient client;
  HTTPClient http;
  if (!http.begin(client, apiUrl(path))) return -1;
  http.setConnectTimeout(3000);
  http.setTimeout(5000);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(body);
  response = http.getString();
  http.end();
  return code;
}

int httpGet(const String &path, String &response) {
  WiFiClient client;
  HTTPClient http;
  if (!http.begin(client, apiUrl(path))) return -1;
  http.setConnectTimeout(3000);
  http.setTimeout(5000);
  int code = http.GET();
  response = http.getString();
  http.end();
  return code;
}

bool announceAndCollect(bool refreshAnnouncement) {
  String response;
  int code = 0;
  if (refreshAnnouncement) {
    JsonDocument request;
    request["hw_id"] = hardwareId;
    request["model"] = "POG Light";
    request["fw_version"] = POGLIGHT_FW_VERSION;
    request["proto_version"] = "1";
    request["name"] = "PogLight";
    request["claim_secret"] = claimSecret;
    String body;
    serializeJson(request, body);
    code = httpPostJson("/api/v1/pogdev/announce", body, response);
    if (code != 202 && code != 200 && code != 409) return false;
  }

  String collectPath = "/api/v1/pogdev/announce/" + hardwareId +
                       "?secret=" + claimSecret;
  code = httpGet(collectPath, response);
  if (code == 404 || code == 410) {
    clearIdentity();
    return false;
  }
  if (code != 200) return false;

  JsonDocument doc;
  if (deserializeJson(doc, response) || doc["status"] != "adopted") return false;
  Credentials next;
  next.deviceId = doc["device_id"].as<String>();
  // L'adresse mDNS déjà résolue est plus fiable que l'en-tête HTTP, qui peut
  // contenir le port de l'API alors que le broker utilise un autre port.
  next.host = poghomeAddress.toString();
  next.port = doc["mqtt"]["port"] | 1883;
  next.password = doc["mqtt"]["password"].as<String>();
  if (!next.valid() || !saveCredentials(next)) return false;
  Serial.printf("[PogHome] adopté comme %s\n", next.deviceId.c_str());
  return true;
}

void rgbToHs(uint32_t packed, float &hue, float &saturation) {
  CRGB rgb((packed >> 16) & 0xff, (packed >> 8) & 0xff, packed & 0xff);
  CHSV hsv = rgb2hsv_approximate(rgb);
  hue = hsv.h * (360.0f / 255.0f);
  saturation = hsv.s * (100.0f / 255.0f);
}

uint32_t hsToRgb(float hue, float saturation) {
  CHSV hsv((uint8_t)constrain(hue * 255.0f / 360.0f, 0.0f, 255.0f),
           (uint8_t)constrain(saturation * 255.0f / 100.0f, 0.0f, 255.0f),
           255);
  CRGB rgb;
  hsv2rgb_rainbow(hsv, rgb);
  return ((uint32_t)rgb.r << 16) | ((uint32_t)rgb.g << 8) | rgb.b;
}

// PubSubClient refuse en silence tout message plus grand que son tampon, et un
// realloc raté laisse `bufferSize` à sa valeur par défaut de 256 octets : la
// lampe se connecte, publie « online », puis n'envoie plus rien. On ne
// redimensionne donc qu'à la hausse, on teste le retour, et on le dit.
bool ensureMqttBuffer(size_t payloadLength) {
  // en-tête fixe + longueur de sujet encodée + marge de sécurité
  size_t needed = payloadLength + 16;
  if (mqtt.getBufferSize() >= needed) return true;
  if (mqtt.setBufferSize(needed)) return true;
  Serial.printf("[PogHome] tampon MQTT trop petit: %u octets refusés\n",
                (unsigned)needed);
  return false;
}

void publishHello() {
  Config snapshot;
  xSemaphoreTake(g_configMutex, portMAX_DELAY);
  snapshot = g_config;
  xSemaphoreGive(g_configMutex);

  JsonDocument doc;
  doc["proto"] = 1;
  doc["hw_id"] = hardwareId;
  doc["model"] = "POG Light";
  doc["fw_version"] = POGLIGHT_FW_VERSION;
  doc["name"] = "PogLight";
  JsonArray entities = doc["entities"].to<JsonArray>();

  const char *purposeLabels[LP_COUNT];
  for (uint8_t i = 0; i < LP_COUNT; ++i) purposeLabels[i] = lightPurposeLabel(i);

  // --- Les quatre de façade -------------------------------------------------
  // Une entité est un état du foyer, qu'on observe ou qu'on commande parce que
  // la maison vit. Ces quatre-là sont les seules que PogHome montrait déjà.
  JsonObject light = entities.add<JsonObject>();
  light["key"] = "light";
  light["name"] = "Éclairage";
  light["category"] = "light";
  JsonArray lightTraits = light["traits"].to<JsonArray>();
  JsonObject masterPower = lightTraits.add<JsonObject>();
  masterPower["id"] = "on_off";
  masterPower["config"]["purpose"] = lightPurposeKey(snapshot.purpose);
  masterPower["config"]["purpose_label"] = lightPurposeLabel(snapshot.purpose);
  lightTraits.add<JsonObject>()["id"] = "brightness";
  lightTraits.add<JsonObject>()["id"] = "color";
  addLightActions(lightTraits, purposeLabels);

  addColorEntity(entities, "accent", "Couleur secondaire", "light");
  addSelectEntity(entities, "effect", "Effet", "light", kEffectNames, TP_COUNT);
  addNumberEntity(entities, "speed", "Vitesse", "light", 0, 100, 1, "%");

  // --- Les six rangées ------------------------------------------------------
  // Réglables à la main, mais hors des surfaces par défaut : le rang tient dans
  // la catégorie « config », le seul jeton que PogHome sache lire aujourd'hui
  // avec « diagnostic ». Ce qui reste ici se règle en regardant le ruban ou
  // parce qu'un habitant le décide (éteindre l'écran la nuit, verrouiller les
  // boutons) — pas parce qu'un tournevis est passé.
  static const char *const directions[] = {"Normal", "Inversé"};
  addSelectEntity(entities, "direction", "Sens du ruban", "config",
                  directions, 2);
  addNumberEntity(entities, "led_count", "Nombre de LED", "config", 1, MAX_LEDS);
  addNumberEntity(entities, "power_limit", "Limite d’alimentation", "config",
                  100, 10000, 100, "mA");
  addSelectEntity(entities, "color_order", "Ordre des couleurs", "config",
                  kColorOrders, ORDER_COUNT);
  addSwitchEntity(entities, "oled_enabled", "Écran OLED", "config");
  addSwitchEntity(entities, "buttons_enabled", "Boutons de la lampe", "config");

  // Le câblage (broche du ruban, broches et adresse de l'écran, broches et type
  // des boutons, type de ruban) et l'appairage Wi-Fi ne sont plus déclarés :
  // ils ne changent que quand un tournevis passe. Ils restent intégralement
  // réglables au portail web embarqué, authentifié — POST /api/config pour le
  // matériel et les sections, POST /api/wifi pour le réseau avec scan.

  JsonObject signal = entities.add<JsonObject>();
  signal["key"] = "wifi_signal";
  signal["name"] = "Signal Wi-Fi";
  signal["category"] = "diagnostic";
  JsonObject signalTrait = signal["traits"].to<JsonArray>().add<JsonObject>();
  signalTrait["id"] = "measurement";
  signalTrait["config"]["unit"] = "dBm";
  signalTrait["config"]["kind"] = "signal_strength";

  // --- Les sections, seulement s'il y en a plus d'une -----------------------
  // Avec une section unique, start=1 et end=numLeds couvrent le ruban entier :
  // la section pilote exactement les mêmes LED que l'entité meneuse, et les
  // huit clés qu'elle ajoute sont huit doublons. À partir de deux sections ce
  // sont de vrais gestes distincts, et elles reviennent à l'identique — mêmes
  // clés, donc mêmes lignes en base, donc noms et identifiants HomeKit
  // conservés. Le découpage se fait désormais au portail web, pas depuis une
  // entité : une entité qui fabrique des entités n'est pas un état du foyer.
  if (snapshot.sectionCount > 1) {
    for (uint8_t i = 0; i < snapshot.sectionCount; ++i) {
      const LedSection& section = snapshot.sections[i];
      JsonObject entity = entities.add<JsonObject>();
      String base = "section_" + String(section.id);
      entity["key"] = base;
      entity["name"] = section.name;
      entity["category"] = "light";
      JsonArray traits = entity["traits"].to<JsonArray>();
      JsonObject power = traits.add<JsonObject>();
      power["id"] = "on_off";
      power["config"]["purpose"] = lightPurposeKey(section.purpose);
      power["config"]["purpose_label"] = lightPurposeLabel(section.purpose);
      power["config"]["start"] = section.start + 1;
      power["config"]["end"] = section.start + section.count;
      traits.add<JsonObject>()["id"] = "brightness";
      traits.add<JsonObject>()["id"] = "color";
      JsonObject sectionEffect = traits.add<JsonObject>();
      sectionEffect["id"] = "select";
      JsonArray sectionOptions = sectionEffect["config"]["options"].to<JsonArray>();
      for (const char *name : kEffectNames) sectionOptions.add(name);

      addSwitchEntity(entities, base + "_enabled", section.name + " · Active", "light");
      addColorEntity(entities, base + "_accent", section.name + " · Couleur secondaire",
                     "light");
      addNumberEntity(entities, base + "_speed", section.name + " · Vitesse",
                      "light", 0, 100, 1, "%");
      // La géométrie et le vocabulaire d'un découpage se posent une fois : rang
      // « config », comme les six rangées de la lampe.
      addTextEntity(entities, base + "_name", section.name + " · Nom", "config");
      addSelectEntity(entities, base + "_purpose", section.name + " · Utilité",
                      "config", purposeLabels, LP_COUNT);
      addNumberEntity(entities, base + "_start", section.name + " · Première LED",
                      "config", 1, snapshot.numLeds);
      addNumberEntity(entities, base + "_end", section.name + " · Dernière LED",
                      "config", 1, snapshot.numLeds);
    }
  }

  doc["local_rules"].to<JsonArray>();
  String payload;
  serializeJson(doc, payload);
  String topic = "pog/" + credentials.deviceId + "/hello";
  bool published = !doc.overflowed() &&
                   ensureMqttBuffer(payload.length() + topic.length()) &&
                   mqtt.publish(topic.c_str(), payload.c_str(), true);
  Serial.printf("[PogHome] manifeste: %u entités, %u octets, %s\n",
                entities.size(), payload.length(), published ? "publié" : "échec");
  helloDirty = !published;
}

void publishState() {
  Config snapshot;
  xSemaphoreTake(g_configMutex, portMAX_DELAY);
  snapshot = g_config;
  xSemaphoreGive(g_configMutex);

  float primaryHue, primarySaturation, accentHue, accentSaturation;
  rgbToHs(snapshot.primaryColor, primaryHue, primarySaturation);
  rgbToHs(snapshot.secondaryColor, accentHue, accentSaturation);
  JsonDocument doc;
  JsonObject light = doc["light"].to<JsonObject>();
  light["on"] = snapshot.pattern != TP_OFF;
  light["brightness"] = roundf(snapshot.brightness * 100.0f / 255.0f);
  light["mode"] = "hs";
  light["hue"] = primaryHue;
  light["saturation"] = primarySaturation;
  JsonObject accent = doc["accent"].to<JsonObject>();
  accent["mode"] = "hs";
  accent["hue"] = accentHue;
  accent["saturation"] = accentSaturation;
  uint8_t effectIndex = snapshot.pattern < TP_COUNT ? snapshot.pattern : TP_RAINBOW;
  doc["effect"]["current"] = kEffectNames[effectIndex];
  doc["speed"]["value"] = snapshot.speed;
  doc["direction"]["current"] = snapshot.reverse ? "Inversé" : "Normal";
  doc["led_count"]["value"] = snapshot.numLeds;
  doc["power_limit"]["value"] = snapshot.maxMilliAmps;
  doc["color_order"]["current"] = kColorOrders[snapshot.colorOrder % ORDER_COUNT];
  doc["oled_enabled"]["on"] = snapshot.oledEnabled;
  doc["buttons_enabled"]["on"] = snapshot.buttonsEnabled;
  doc["wifi_signal"]["value"] = WiFi.RSSI();
  doc["wifi_signal"]["kind"] = "signal_strength";
  // L'utilité ne sort plus par ici : elle voyage dans le `config` du trait
  // on_off de « light », republié par set_purpose qui pose schemaChanged.
  // La garde doit être littéralement la même que dans publishHello : un état
  // publié sans entité déclarée reste retenu sur le bus sans que personne
  // ne le voie, et une entité déclarée sans état n'affiche rien.
  if (snapshot.sectionCount > 1) {
    for (uint8_t i = 0; i < snapshot.sectionCount; ++i) {
      const LedSection& section = snapshot.sections[i];
      String key = "section_" + String(section.id);
      JsonObject state = doc[key].to<JsonObject>();
      state["on"] = section.enabled && section.on && snapshot.pattern != TP_OFF;
      state["brightness"] = roundf(section.brightness * 100.0f / 255.0f);
      float hue, saturation;
      rgbToHs(section.primaryColor, hue, saturation);
      state["mode"] = "hs";
      state["hue"] = hue;
      state["saturation"] = saturation;
      uint8_t sectionPattern = section.pattern < TP_COUNT ? section.pattern : TP_SOLID;
      state["current"] = kEffectNames[sectionPattern];
      state["purpose"] = lightPurposeKey(section.purpose);
      doc[key + "_enabled"]["on"] = section.enabled;
      doc[key + "_name"]["value"] = section.name;
      doc[key + "_purpose"]["current"] = lightPurposeLabel(section.purpose);
      doc[key + "_start"]["value"] = section.start + 1;
      doc[key + "_end"]["value"] = section.start + section.count;
      float sectionAccentHue, sectionAccentSaturation;
      rgbToHs(section.secondaryColor, sectionAccentHue, sectionAccentSaturation);
      JsonObject sectionAccent = doc[key + "_accent"].to<JsonObject>();
      sectionAccent["mode"] = "hs";
      sectionAccent["hue"] = sectionAccentHue;
      sectionAccent["saturation"] = sectionAccentSaturation;
      doc[key + "_speed"]["value"] = section.speed;
    }
  }

  String payload;
  serializeJson(doc, payload);
  String topic = "pog/" + credentials.deviceId + "/state";
  // Le retour était ignoré : un état perdu ne revenait qu'au cycle de 30 s.
  stateDirty = !(!doc.overflowed() &&
                 ensureMqttBuffer(payload.length() + topic.length()) &&
                 mqtt.publish(topic.c_str(), payload.c_str(), true));
}

void saveChangedConfig(bool schemaChanged = false, bool requiresReboot = false) {
  configSave();
  stateDirty = true;
  if (schemaChanged) helloDirty = true;
  if (requiresReboot) rebootAt = millis() + 1200;
}

bool setSwitchCommand(const String &name, bool &value) {
  if (name == "turn_on") value = true;
  else if (name == "turn_off") value = false;
  else if (name == "toggle") value = !value;
  else return false;
  return true;
}

bool hardwarePinsValid(const Config &config) {
  if (config.oledEnabled && config.oledSda == config.oledScl) return false;
  uint8_t used[11];
  uint8_t count = 0;
  used[count++] = config.ledPin;
  if (config.oledEnabled) {
    used[count++] = config.oledSda;
    used[count++] = config.oledScl;
  }
  if (config.buttonsEnabled) {
#if !SOC_TOUCH_SENSOR_SUPPORTED
    if (config.buttonMode == BIM_CAPACITIVE) return false;
#endif
    for (uint8_t pin : config.buttonPins) used[count++] = pin;
  }
  for (uint8_t i = 0; i < count; ++i) {
    for (uint8_t j = i + 1; j < count; ++j) {
      if (used[i] == used[j]) return false;
    }
  }
  return true;
}

void clampSectionsToStrip(Config &config) {
  for (uint8_t i = 0; i < config.sectionCount; ++i) {
    LedSection &section = config.sections[i];
    section.start = min(section.start, (uint16_t)(config.numLeds - 1));
    section.count = constrain(section.count, (uint16_t)1,
                              (uint16_t)(config.numLeds - section.start));
  }
}

void handleCommand(char *, byte *payload, unsigned int length) {
  JsonDocument doc;
  if (deserializeJson(doc, payload, length)) return;
  String key = doc["key"].as<String>();
  String name = doc["name"].as<String>();
  JsonObjectConst params = doc["params"].as<JsonObjectConst>();
  bool changed = false;
  bool schemaChanged = false;
  bool requiresReboot = false;

  xSemaphoreTake(g_configMutex, portMAX_DELAY);
  Config before = g_config;
  if (key == "light") {
    if (name == "sync_effect") {
      // Reprise de l'ancienne entité « room_sync », devenue une commande de la
      // lampe : un geste de la lampe n'est pas un objet de plus dans la maison.
      String option = params["effect"].as<String>();
      for (uint8_t i = 0; i < TP_COUNT; ++i) {
        if (option != kEffectNames[i]) continue;
        g_config.pattern = i;
        g_config.speed = constrain((int)(params["speed"] | 50), 0, 100);
        g_config.brightness = (uint8_t)roundf(
            constrain((float)(params["brightness"] | 100.0f), 0.0f, 100.0f) *
            2.55f);
        g_config.primaryColor =
            hsToRgb(params["primary_hue"] | 0.0f,
                    params["primary_saturation"] | 100.0f);
        g_config.secondaryColor =
            hsToRgb(params["secondary_hue"] | 240.0f,
                    params["secondary_saturation"] | 100.0f);
        if (i != TP_OFF) lastActivePattern = i;
        for (uint8_t section = 0; section < g_config.sectionCount; ++section) {
          LedSection &target = g_config.sections[section];
          target.pattern = i;
          target.speed = g_config.speed;
          target.primaryColor = g_config.primaryColor;
          target.secondaryColor = g_config.secondaryColor;
          target.on = i != TP_OFF;
        }
        changed = true;
        break;
      }
    } else if (name == "set_purpose") {
      // Reprise de l'ancienne entité « purpose ». schemaChanged est le seul
      // chemin par lequel la valeur voyage encore : elle est republiée dans le
      // `config` du trait on_off, et n'existe plus dans l'état.
      int purpose = purposeFromLabel(params["purpose"].as<String>());
      if (purpose >= 0) {
        g_config.purpose = purpose;
        changed = schemaChanged = true;
      }
    } else if (name == "turn_off") {
      if (g_config.pattern != TP_OFF) lastActivePattern = g_config.pattern;
      g_config.pattern = TP_OFF;
      changed = true;
    } else if (name == "turn_on") {
      if (g_config.pattern == TP_OFF) g_config.pattern = lastActivePattern;
      changed = true;
    } else if (name == "toggle") {
      if (g_config.pattern == TP_OFF) g_config.pattern = lastActivePattern;
      else {
        lastActivePattern = g_config.pattern;
        g_config.pattern = TP_OFF;
      }
      changed = true;
    } else if (name == "set_brightness") {
      float value = params["brightness"] | 0.0f;
      g_config.brightness = (uint8_t)roundf(constrain(value, 0.0f, 100.0f) * 255.0f / 100.0f);
      changed = true;
    } else if (name == "set_hs") {
      g_config.primaryColor = hsToRgb(params["hue"] | 0.0f, params["saturation"] | 0.0f);
      for (uint8_t i = 0; i < g_config.sectionCount; ++i) {
        g_config.sections[i].primaryColor = g_config.primaryColor;
      }
      changed = true;
    }
  } else if (key == "accent" && name == "set_hs") {
    g_config.secondaryColor = hsToRgb(params["hue"] | 0.0f, params["saturation"] | 0.0f);
    for (uint8_t i = 0; i < g_config.sectionCount; ++i) {
      g_config.sections[i].secondaryColor = g_config.secondaryColor;
    }
    changed = true;
  } else if (key == "speed" && name == "set_value") {
    g_config.speed = constrain((int)(params["value"] | 0), 0, 100);
    for (uint8_t i = 0; i < g_config.sectionCount; ++i) {
      g_config.sections[i].speed = g_config.speed;
    }
    changed = true;
  } else if (key == "direction" && name == "select_option") {
    String option = params["option"].as<String>();
    if (option == "Normal" || option == "Inversé") {
      g_config.reverse = option == "Inversé";
      changed = true;
    }
  } else if (key == "effect" && name == "select_option") {
    String option = params["option"].as<String>();
    for (uint8_t i = 0; i < TP_COUNT; ++i) {
      if (option == kEffectNames[i]) {
        g_config.pattern = i;
        if (i != TP_OFF) lastActivePattern = i;
        if (i != TP_OFF) {
          for (uint8_t section = 0; section < g_config.sectionCount; ++section) {
            g_config.sections[section].pattern = i;
            g_config.sections[section].on = true;
          }
        }
        changed = true;
        break;
      }
    }
  } else if (key == "led_count" && name == "set_value") {
    g_config.numLeds = constrain((int)(params["value"] | 1), 1, MAX_LEDS);
    clampSectionsToStrip(g_config);
    changed = schemaChanged = requiresReboot = true;
  } else if (key == "power_limit" && name == "set_value") {
    g_config.maxMilliAmps = constrain((int)(params["value"] | 100), 100, 10000);
    changed = true;
  } else if (key == "color_order" && name == "select_option") {
    String option = params["option"].as<String>();
    for (uint8_t i = 0; i < ORDER_COUNT; ++i) {
      if (option == kColorOrders[i]) {
        g_config.colorOrder = i;
        changed = true;
        break;
      }
    }
  } else if (key == "oled_enabled") {
    changed = setSwitchCommand(name, g_config.oledEnabled);
    requiresReboot = changed;
  } else if (key == "buttons_enabled") {
    changed = setSwitchCommand(name, g_config.buttonsEnabled);
    requiresReboot = changed;
  } else if (key.startsWith("section_")) {
    int suffixAt = key.indexOf('_', 8);
    uint16_t id = key.substring(8, suffixAt < 0 ? key.length() : suffixAt).toInt();
    String suffix = suffixAt < 0 ? "" : key.substring(suffixAt + 1);
    for (uint8_t i = 0; i < g_config.sectionCount; ++i) {
      LedSection& section = g_config.sections[i];
      if (section.id != id) continue;
      if (suffix == "" && name == "turn_off") {
        section.on = false;
        changed = true;
      } else if (suffix == "" && name == "turn_on") {
        section.on = true;
        changed = true;
      } else if (suffix == "" && name == "toggle") {
        section.on = !section.on;
        changed = true;
      } else if (suffix == "" && name == "set_brightness") {
        float value = params["brightness"] | 0.0f;
        section.brightness = (uint8_t)roundf(constrain(value, 0.0f, 100.0f) * 255.0f / 100.0f);
        changed = true;
      } else if (suffix == "" && name == "set_hs") {
        section.primaryColor = hsToRgb(params["hue"] | 0.0f, params["saturation"] | 0.0f);
        changed = true;
      } else if (suffix == "" && name == "select_option") {
        String option = params["option"].as<String>();
        for (uint8_t pattern = 0; pattern < TP_COUNT; ++pattern) {
          if (option == kEffectNames[pattern]) {
            section.pattern = pattern;
            section.on = pattern != TP_OFF;
            changed = true;
            break;
          }
        }
      } else if (suffix == "enabled") {
        changed = setSwitchCommand(name, section.enabled);
      } else if (suffix == "name" && name == "set_text") {
        String value = params["value"].as<String>();
        value.trim();
        if (value.length() && value.length() <= 32) {
          section.name = value;
          changed = schemaChanged = true;
        }
      } else if (suffix == "purpose" && name == "select_option") {
        int purpose = purposeFromLabel(params["option"].as<String>());
        if (purpose >= 0) {
          section.purpose = purpose;
          changed = schemaChanged = true;
        }
      } else if ((suffix == "start" || suffix == "end") && name == "set_value") {
        int value = constrain((int)(params["value"] | 1), 1, (int)g_config.numLeds);
        uint16_t currentEnd = section.start + section.count;
        if (suffix == "start") {
          section.start = value - 1;
          section.count = max((uint16_t)1,
                              (uint16_t)(currentEnd > section.start
                                             ? currentEnd - section.start
                                             : 1));
          section.count = min(section.count,
                              (uint16_t)(g_config.numLeds - section.start));
        } else {
          uint16_t end = max((uint16_t)value, (uint16_t)(section.start + 1));
          section.count = end - section.start;
        }
        changed = schemaChanged = true;
      } else if (suffix == "accent" && name == "set_hs") {
        section.secondaryColor =
            hsToRgb(params["hue"] | 0.0f, params["saturation"] | 0.0f);
        changed = true;
      } else if (suffix == "speed" && name == "set_value") {
        section.speed = constrain((int)(params["value"] | 0), 0, 100);
        changed = true;
      }
      break;
    }
  }
  // Les broches ne sont plus déclarées, mais oled_enabled et buttons_enabled
  // restent écrivables : allumer un périphérique dont les broches entrent en
  // collision avec la sortie LED annule tout. La correction se fait désormais
  // au portail web uniquement, donc le refus doit au moins être visible — on le
  // trace et on republie l'état pour que l'interrupteur revienne à sa position
  // réelle au lieu de paraître accepté.
  bool reverted = false;
  if (changed && !hardwarePinsValid(g_config)) {
    g_config = before;
    changed = schemaChanged = requiresReboot = false;
    reverted = true;
  }
  if (changed) saveChangedConfig(schemaChanged, requiresReboot);
  xSemaphoreGive(g_configMutex);

  if (reverted) {
    Serial.printf("[PogHome] %s refusé: collision de broches, à corriger sur "
                  "http://%s.local\n", key.c_str(), MDNS_NAME);
  }
  if ((changed || reverted) && mqtt.connected()) {
    if (schemaChanged) publishHello();
    publishState();
  }
}

bool connectMqtt() {
  mqtt.setServer(credentials.host.c_str(), credentials.port);
  mqtt.setCallback(handleCommand);
  // Le manifeste réduit tient largement dans 8 Kio avec une seule section.
  // ensureMqttBuffer l’agrandit à la demande si l’habitant découpe le ruban
  // depuis le portail — jamais à la baisse en cours de session, et jamais sans
  // vérifier le retour. Les 24 Kio d’avant étaient calibrés sur un pire cas à
  // huit sections que personne n’a, et étaient réservés à chaque connexion.
  ensureMqttBuffer(8192);
  mqtt.setKeepAlive(30);
  String statusTopic = "pog/" + credentials.deviceId + "/status";
  bool ok = mqtt.connect(credentials.deviceId.c_str(), credentials.deviceId.c_str(),
                         credentials.password.c_str(), statusTopic.c_str(), 1,
                         true, "offline", true);
  if (!ok) {
    if (mqtt.state() == MQTT_CONNECT_UNAUTHORIZED ||
        mqtt.state() == MQTT_CONNECT_BAD_CREDENTIALS) {
      Serial.println("[PogHome] identifiants refusés, nouvelle adoption requise");
      clearIdentity();
    }
    return false;
  }
  String cmdTopic = "pog/" + credentials.deviceId + "/cmd";
  mqtt.subscribe(cmdTopic.c_str(), 1);
  mqtt.publish(statusTopic.c_str(), "online", true);
  publishHello();
  publishState();
  Serial.println("[PogHome] MQTT connecté");
  return true;
}

void pogdevTask(void *) {
  loadIdentity();
  hardwareId = makeHardwareId();
  uint32_t nextEnrolment = 0;
  uint32_t nextReconnect = 0;
  uint32_t nextState = 0;
  uint32_t nextHelloAttempt = 0;
  uint32_t nextStateAttempt = 0;
  uint32_t helloBackoffMs = kMinBackoffMs;
  uint32_t stateBackoffMs = kMinBackoffMs;
  uint32_t enrolmentStarted = millis();
  bool refreshAnnouncement = true;
  uint32_t nextRediscovery = 0;

  for (;;) {
    uint32_t now = millis();
    if (WiFi.status() != WL_CONNECTED) {
      mqtt.disconnect();
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    if (!credentials.valid()) {
      if ((int32_t)(now - nextEnrolment) >= 0) {
        if ((poghomeAddress || discoverPogHome()) &&
            announceAndCollect(refreshAnnouncement)) {
          nextReconnect = 0;
        } else {
          poghomeAddress = IPAddress();
        }
        // On alterne annonce et relève pendant la phase rapide : détection en
        // 5 s sans dépasser la limite de 20 requêtes/minute de PogHome.
        refreshAnnouncement = !refreshAnnouncement;
        uint32_t elapsed = now - enrolmentStarted;
        uint32_t interval = elapsed < 60000 ? 5000 :
                            elapsed < 3600000 ? 30000 : 300000;
        nextEnrolment = now + interval;
      }
    } else if (!mqtt.connected()) {
      if ((int32_t)(now - nextReconnect) >= 0) {
        if (!connectMqtt() && credentials.valid() &&
            (int32_t)(now - nextRediscovery) >= 0) {
          // Le bail DHCP de PogHome peut changer sans invalider l'adoption.
          poghomeAddress = IPAddress();
          if (discoverPogHome()) {
            credentials.host = poghomeAddress.toString();
            saveCredentials(credentials);
          }
          nextRediscovery = now + 30000;
        }
        nextReconnect = now + kReconnectPeriodMs;
      }
    } else {
      mqtt.loop();
      // Une publication ratée reconstruisait le manifeste entier cinquante fois
      // par seconde, indéfiniment : on réessaie avec un délai qui double.
      if (helloDirty && (int32_t)(now - nextHelloAttempt) >= 0) {
        publishHello();
        helloBackoffMs = helloDirty ? min(helloBackoffMs * 2, kMaxBackoffMs)
                                    : kMinBackoffMs;
        nextHelloAttempt = now + helloBackoffMs;
      }
      // Même garde côté état, sans quoi tester le retour de mqtt.publish
      // créerait exactement la boucle que l’on vient de fermer.
      if ((stateDirty && (int32_t)(now - nextStateAttempt) >= 0) ||
          (int32_t)(now - nextState) >= 0) {
        publishState();
        stateBackoffMs = stateDirty ? min(stateBackoffMs * 2, kMaxBackoffMs)
                                    : kMinBackoffMs;
        nextStateAttempt = now + stateBackoffMs;
        nextState = now + kStatePeriodMs;
      }
    }
    if (rebootAt && (int32_t)(now - rebootAt) >= 0) {
      mqtt.loop();
      delay(80);
      ESP.restart();
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

}  // namespace

void pogdevBegin() {
  if (taskHandle) return;
  xTaskCreate(pogdevTask, "pogdev", 8192, nullptr, 1, &taskHandle);
}

void pogdevNotifyState() {
  stateDirty = true;
}

void pogdevNotifyConfig() {
  helloDirty = true;
  stateDirty = true;
}
