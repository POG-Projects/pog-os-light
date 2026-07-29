#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <soc/soc_caps.h>
#include "config.h"
#include "leds.h"
#include "display.h"
#include "web_ui.h"
#include "poglight_icon.h"
#include "web.h"
#include "pogdev.h"

static WebServer server(80);
static DNSServer dns;
static bool      s_apMode = false;
static const byte DNS_PORT = 53;
static const IPAddress AP_IP(192, 168, 4, 1);

static void handleRoot() {
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html", INDEX_HTML);
}

static void handleIcon() {
  server.sendHeader("Cache-Control", "public, max-age=86400");
  server.send_P(200, "image/png", reinterpret_cast<PGM_P>(POGLIGHT_ICON), POGLIGHT_ICON_LEN);
}

static void handleManifest() {
  static const char manifest[] PROGMEM =
      "{\"name\":\"POG Light\",\"short_name\":\"PogLight\","
      "\"start_url\":\"/\",\"display\":\"standalone\","
      "\"background_color\":\"#07080d\",\"theme_color\":\"#07080d\","
      "\"icons\":[{\"src\":\"/icon.png\",\"sizes\":\"256x256\",\"type\":\"image/png\"}]}";
  server.sendHeader("Cache-Control", "public, max-age=86400");
  server.send_P(200, "application/manifest+json", manifest);
}

static void handleState() {
  JsonDocument doc;
#if CONFIG_IDF_TARGET_ESP32C3
  doc["board"]  = "esp32c3";
#elif CONFIG_IDF_TARGET_ESP32S3
  doc["board"]  = "esp32s3";
#else
  doc["board"]  = "esp32";
#endif
  doc["apMode"] = s_apMode;
  doc["ip"]     = s_apMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  doc["oledFound"]   = g_oledFound;
  doc["oledAddr"]    = g_oledAddr;
  doc["oledSwapped"] = g_oledSwapped;
  doc["touchSupported"] = SOC_TOUCH_SENSOR_SUPPORTED;
  xSemaphoreTake(g_configMutex, portMAX_DELAY);
  configFillJson(doc["config"].to<JsonObject>(), false);
  xSemaphoreGive(g_configMutex);
  String out; serializeJson(doc, out);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", out);
}

static void handleSaveConfig() {
  if (!server.hasArg("plain")) { server.send(400, "application/json", "{\"ok\":false}"); return; }
  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) { server.send(400, "application/json", "{\"ok\":false}"); return; }
  xSemaphoreTake(g_configMutex, portMAX_DELAY);
  Config before = g_config;
  configApplyJson(doc.as<JsonObjectConst>());
  bool hw = g_config.ledPin != before.ledPin ||
            g_config.numLeds != before.numLeds ||
            g_config.analog != before.analog ||
            g_config.oledEnabled != before.oledEnabled ||
            g_config.oledSda != before.oledSda ||
            g_config.oledScl != before.oledScl ||
            g_config.oledAddress != before.oledAddress ||
            g_config.buttonsEnabled != before.buttonsEnabled ||
            g_config.buttonMode != before.buttonMode;
  for (uint8_t i = 0; i < 4; ++i) {
    hw = hw || g_config.buttonPins[i] != before.buttonPins[i];
  }
  configSave();
  xSemaphoreGive(g_configMutex);
  pogdevNotifyConfig();
  if (hw) { server.send(200, "application/json", "{\"ok\":true,\"reboot\":true}"); delay(500); ESP.restart(); }
  else server.send(200, "application/json", "{\"ok\":true}");
}

static void handleWifi() {
  if (!server.hasArg("plain")) { server.send(400, "application/json", "{\"ok\":false}"); return; }
  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) { server.send(400, "application/json", "{\"ok\":false}"); return; }
  xSemaphoreTake(g_configMutex, portMAX_DELAY);
  if (doc["wifiSsid"].is<const char*>()) g_config.wifiSsid = doc["wifiSsid"].as<String>();
  if (doc["wifiPass"].is<const char*>()) g_config.wifiPass = doc["wifiPass"].as<String>();
  configSave();
  xSemaphoreGive(g_configMutex);
  server.send(200, "application/json", "{\"ok\":true,\"reboot\":true}");
  delay(400); ESP.restart();
}

// Premier démarrage : matériel + Wi-Fi sont enregistrés ensemble pour éviter
// deux redémarrages au milieu de l'onboarding mobile.
static void handleSetup() {
  if (!server.hasArg("plain")) { server.send(400, "application/json", "{\"ok\":false,\"error\":\"body requis\"}"); return; }
  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) { server.send(400, "application/json", "{\"ok\":false,\"error\":\"json invalide\"}"); return; }
  if (!doc["wifiSsid"].is<const char*>() || !strlen(doc["wifiSsid"].as<const char*>())) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"wifi requis\"}");
    return;
  }
  xSemaphoreTake(g_configMutex, portMAX_DELAY);
  configApplyJson(doc.as<JsonObjectConst>());
  g_config.wifiSsid = doc["wifiSsid"].as<String>();
  if (doc["wifiPass"].is<const char*>()) g_config.wifiPass = doc["wifiPass"].as<String>();
  configSave();
  xSemaphoreGive(g_configMutex);
  server.send(200, "application/json", "{\"ok\":true,\"reboot\":true}");
  delay(1800);
  ESP.restart();
}

static void handleScan() {
  int n = WiFi.scanNetworks();
  JsonDocument doc; JsonArray arr = doc.to<JsonArray>();
  for (int i = 0; i < n; i++) {
    JsonObject o = arr.add<JsonObject>();
    o["ssid"] = WiFi.SSID(i); o["rssi"] = WiFi.RSSI(i); o["lock"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
  }
  WiFi.scanDelete();
  String out; serializeJson(doc, out);
  server.send(200, "application/json", out);
}

static void handleLeds() {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "text/plain", ledsSnapshot());
}

static void handleReboot() { server.send(200, "application/json", "{\"ok\":true}"); delay(300); ESP.restart(); }

static void handleOtaDone() {
  bool ok = !Update.hasError();
  server.sendHeader("Connection", "close");
  server.send(200, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"echec maj\"}");
  if (ok) { delay(800); ESP.restart(); }
}
static void handleOtaUpload() {
  HTTPUpload& up = server.upload();
  if (up.status == UPLOAD_FILE_START)      Update.begin(UPDATE_SIZE_UNKNOWN);
  else if (up.status == UPLOAD_FILE_WRITE) Update.write(up.buf, up.currentSize);
  else if (up.status == UPLOAD_FILE_END)   Update.end(true);
  else if (up.status == UPLOAD_FILE_ABORTED) Update.abort();
}

static void handleNotFound() {
  if (s_apMode) { server.sendHeader("Location", String("http://") + AP_IP.toString(), true); server.send(302, "text/plain", ""); return; }
  server.send(404, "text/plain", "404");
}

void webBegin(bool apMode) {
  s_apMode = apMode;
  if (apMode) dns.start(DNS_PORT, "*", AP_IP);
  server.on("/",            HTTP_GET,  handleRoot);
  server.on("/icon.png",    HTTP_GET,  handleIcon);
  server.on("/manifest.webmanifest", HTTP_GET, handleManifest);
  server.on("/api/state",   HTTP_GET,  handleState);
  server.on("/api/leds",    HTTP_GET,  handleLeds);
  server.on("/api/scan",    HTTP_GET,  handleScan);
  server.on("/api/config",  HTTP_POST, handleSaveConfig);
  server.on("/api/wifi",    HTTP_POST, handleWifi);
  server.on("/api/setup",   HTTP_POST, handleSetup);
  server.on("/api/reboot",  HTTP_POST, handleReboot);
  server.on("/api/ota",     HTTP_POST, handleOtaDone, handleOtaUpload);
  server.on("/generate_204", handleNotFound);
  server.on("/gen_204",      handleNotFound);
  server.onNotFound(handleNotFound);
  server.begin();
}

void webLoop() {
  if (s_apMode) dns.processNextRequest();
  server.handleClient();
}
