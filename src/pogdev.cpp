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
uint8_t lastActivePattern = TP_RAINBOW;

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

void publishHello() {
  JsonDocument doc;
  doc["proto"] = 1;
  doc["hw_id"] = hardwareId;
  doc["model"] = "POG Light";
  doc["fw_version"] = POGLIGHT_FW_VERSION;
  doc["name"] = "PogLight";
  JsonArray entities = doc["entities"].to<JsonArray>();

  JsonObject light = entities.add<JsonObject>();
  light["key"] = "light";
  light["name"] = "Éclairage";
  light["category"] = "light";
  JsonArray lightTraits = light["traits"].to<JsonArray>();
  lightTraits.add<JsonObject>()["id"] = "on_off";
  lightTraits.add<JsonObject>()["id"] = "brightness";
  lightTraits.add<JsonObject>()["id"] = "color";

  JsonObject accent = entities.add<JsonObject>();
  accent["key"] = "accent";
  accent["name"] = "Couleur secondaire";
  accent["category"] = "light";
  accent["traits"].to<JsonArray>().add<JsonObject>()["id"] = "color";

  JsonObject effect = entities.add<JsonObject>();
  effect["key"] = "effect";
  effect["name"] = "Effet";
  effect["category"] = "light";
  JsonObject effectTrait = effect["traits"].to<JsonArray>().add<JsonObject>();
  effectTrait["id"] = "select";
  JsonArray options = effectTrait["config"]["options"].to<JsonArray>();
  const char *effectNames[TP_COUNT] = {
      "Uni", "Ordre couleurs", "Pixel mobile", "Remplissage", "Arc-en-ciel",
      "Chenillard", "Respiration", "Feu", "Scintillement", "Dégradé",
      "Balayage", "Blanc", "Éteint"};
  for (const char *name : effectNames) options.add(name);

  JsonObject speed = entities.add<JsonObject>();
  speed["key"] = "speed";
  speed["name"] = "Vitesse";
  speed["category"] = "light";
  JsonObject speedTrait = speed["traits"].to<JsonArray>().add<JsonObject>();
  speedTrait["id"] = "number";
  speedTrait["config"]["min"] = 0;
  speedTrait["config"]["max"] = 100;
  speedTrait["config"]["step"] = 1;
  speedTrait["config"]["unit"] = "%";

  JsonObject direction = entities.add<JsonObject>();
  direction["key"] = "direction";
  direction["name"] = "Sens";
  direction["category"] = "config";
  JsonObject directionTrait = direction["traits"].to<JsonArray>().add<JsonObject>();
  directionTrait["id"] = "select";
  JsonArray directionOptions = directionTrait["config"]["options"].to<JsonArray>();
  directionOptions.add("Normal");
  directionOptions.add("Inversé");

  JsonObject signal = entities.add<JsonObject>();
  signal["key"] = "wifi_signal";
  signal["name"] = "Signal Wi-Fi";
  signal["category"] = "diagnostic";
  JsonObject signalTrait = signal["traits"].to<JsonArray>().add<JsonObject>();
  signalTrait["id"] = "measurement";
  signalTrait["config"]["unit"] = "dBm";
  signalTrait["config"]["kind"] = "signal_strength";

  doc["local_rules"].to<JsonArray>();
  String payload;
  serializeJson(doc, payload);
  String topic = "pog/" + credentials.deviceId + "/hello";
  mqtt.publish(topic.c_str(), payload.c_str(), true);
}

void publishState() {
  Config snapshot;
  xSemaphoreTake(g_configMutex, portMAX_DELAY);
  snapshot = g_config;
  xSemaphoreGive(g_configMutex);

  float primaryHue, primarySaturation, accentHue, accentSaturation;
  rgbToHs(snapshot.primaryColor, primaryHue, primarySaturation);
  rgbToHs(snapshot.secondaryColor, accentHue, accentSaturation);
  const char *effectNames[TP_COUNT] = {
      "Uni", "Ordre couleurs", "Pixel mobile", "Remplissage", "Arc-en-ciel",
      "Chenillard", "Respiration", "Feu", "Scintillement", "Dégradé",
      "Balayage", "Blanc", "Éteint"};

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
  doc["effect"]["current"] = effectNames[effectIndex];
  doc["speed"]["value"] = snapshot.speed;
  doc["direction"]["current"] = snapshot.reverse ? "Inversé" : "Normal";
  doc["wifi_signal"]["value"] = WiFi.RSSI();
  doc["wifi_signal"]["kind"] = "signal_strength";

  String payload;
  serializeJson(doc, payload);
  String topic = "pog/" + credentials.deviceId + "/state";
  mqtt.publish(topic.c_str(), payload.c_str(), true);
  stateDirty = false;
}

void saveChangedConfig() {
  configSave();
  stateDirty = true;
}

void handleCommand(char *, byte *payload, unsigned int length) {
  JsonDocument doc;
  if (deserializeJson(doc, payload, length)) return;
  String key = doc["key"].as<String>();
  String name = doc["name"].as<String>();
  JsonObjectConst params = doc["params"].as<JsonObjectConst>();
  bool changed = false;

  xSemaphoreTake(g_configMutex, portMAX_DELAY);
  if (key == "light") {
    if (name == "turn_off") {
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
      changed = true;
    }
  } else if (key == "accent" && name == "set_hs") {
    g_config.secondaryColor = hsToRgb(params["hue"] | 0.0f, params["saturation"] | 0.0f);
    changed = true;
  } else if (key == "speed" && name == "set_value") {
    g_config.speed = constrain((int)(params["value"] | 0), 0, 100);
    changed = true;
  } else if (key == "direction" && name == "select_option") {
    String option = params["option"].as<String>();
    if (option == "Normal" || option == "Inversé") {
      g_config.reverse = option == "Inversé";
      changed = true;
    }
  } else if (key == "effect" && name == "select_option") {
    const char *effectNames[TP_COUNT] = {
        "Uni", "Ordre couleurs", "Pixel mobile", "Remplissage", "Arc-en-ciel",
        "Chenillard", "Respiration", "Feu", "Scintillement", "Dégradé",
        "Balayage", "Blanc", "Éteint"};
    String option = params["option"].as<String>();
    for (uint8_t i = 0; i < TP_COUNT; ++i) {
      if (option == effectNames[i]) {
        g_config.pattern = i;
        if (i != TP_OFF) lastActivePattern = i;
        changed = true;
        break;
      }
    }
  }
  if (changed) saveChangedConfig();
  xSemaphoreGive(g_configMutex);

  if (changed && mqtt.connected()) publishState();
}

bool connectMqtt() {
  mqtt.setServer(credentials.host.c_str(), credentials.port);
  mqtt.setCallback(handleCommand);
  mqtt.setBufferSize(4096);
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
      if (stateDirty || (int32_t)(now - nextState) >= 0) {
        publishState();
        nextState = now + kStatePeriodMs;
      }
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
