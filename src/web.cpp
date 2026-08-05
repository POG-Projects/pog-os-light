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
#include "web_auth.h"
#include "pogdev.h"
#include "ota_update.h"

static WebServer server(80);
static DNSServer dns;
static bool      s_apMode = false;
static const byte DNS_PORT = 53;
static const IPAddress AP_IP(192, 168, 4, 1);
static bool otaUploadAuthorized = false;
static uint8_t loginFailures = 0;
static uint32_t loginLockoutUntil = 0;

static bool requestAuthorized() {
  return webAuthHasPassword() &&
         webAuthTokenValid(server.header("X-Auth-Token"));
}

static bool requireAuth() {
  if (requestAuthorized()) return true;
  server.sendHeader("Cache-Control", "no-store");
  server.send(401, "application/json", "{\"error\":\"unauthorized\"}");
  return false;
}

static void handleAuthStatus() {
  JsonDocument doc;
  doc["hasPassword"] = webAuthHasPassword();
  doc["authed"] = requestAuthorized();
  String out;
  serializeJson(doc, out);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", out);
}

static void handleAuthSetup() {
  if (webAuthHasPassword()) {
    server.send(403, "application/json",
                "{\"success\":false,\"error\":\"already_set\"}");
    return;
  }
  if (!server.hasArg("plain") || server.arg("plain").length() > 256) {
    server.send(400, "application/json", "{\"error\":\"invalid request\"}");
    return;
  }
  JsonDocument request;
  if (deserializeJson(request, server.arg("plain")) ||
      !webAuthSetPassword(request["password"].as<String>())) {
    server.send(400, "application/json",
                "{\"error\":\"password must contain 8 to 128 characters\"}");
    return;
  }
  JsonDocument response;
  response["success"] = true;
  response["token"] = webAuthIssueToken();
  String out;
  serializeJson(response, out);
  server.send(200, "application/json", out);
}

static void handleAuthLogin() {
  uint32_t now = millis();
  if (!webAuthHasPassword()) {
    server.send(400, "application/json", "{\"error\":\"no_password\"}");
    return;
  }
  if (loginLockoutUntil && (int32_t)(now - loginLockoutUntil) < 0) {
    server.sendHeader("Retry-After", "30");
    server.send(429, "application/json", "{\"error\":\"locked_out\"}");
    return;
  }
  if (!server.hasArg("plain") || server.arg("plain").length() > 256) {
    server.send(400, "application/json", "{\"error\":\"invalid request\"}");
    return;
  }
  JsonDocument request;
  if (deserializeJson(request, server.arg("plain")) ||
      !webAuthCheckPassword(request["password"].as<String>())) {
    delay(300);
    if (++loginFailures >= 5) {
      loginFailures = 0;
      loginLockoutUntil = millis() + 30000;
    }
    server.send(401, "application/json", "{\"success\":false}");
    return;
  }
  loginFailures = 0;
  loginLockoutUntil = 0;
  JsonDocument response;
  response["success"] = true;
  response["token"] = webAuthIssueToken();
  String out;
  serializeJson(response, out);
  server.send(200, "application/json", out);
}

static void handleAuthPassword() {
  if (!requireAuth()) return;
  if (!server.hasArg("plain") || server.arg("plain").length() > 384) {
    server.send(400, "application/json", "{\"error\":\"invalid request\"}");
    return;
  }
  JsonDocument request;
  if (deserializeJson(request, server.arg("plain")) ||
      !webAuthCheckPassword(request["current_password"].as<String>())) {
    delay(300);
    server.send(403, "application/json", "{\"error\":\"current password refused\"}");
    return;
  }
  if (!webAuthSetPassword(request["new_password"].as<String>())) {
    server.send(400, "application/json", "{\"error\":\"invalid new password\"}");
    return;
  }
  webAuthInvalidateTokens();
  JsonDocument response;
  response["success"] = true;
  response["token"] = webAuthIssueToken();
  String out;
  serializeJson(response, out);
  server.send(200, "application/json", out);
}

static void handleAuthLogout() {
  if (!requireAuth()) return;
  webAuthRevokeToken(server.header("X-Auth-Token"));
  server.send(200, "application/json", "{\"success\":true}");
}

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
  if (!requireAuth()) return;
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
  if (!requireAuth()) return;
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
  if (!requireAuth()) return;
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
  if (!requireAuth()) return;
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
  if (!requireAuth()) return;
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
  if (!requireAuth()) return;
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "text/plain", ledsSnapshot());
}

static void handleReboot() {
  if (!requireAuth()) return;
  server.send(200, "application/json", "{\"ok\":true}");
  delay(300);
  ESP.restart();
}

static void handleUpdateStatus() {
  if (!requireAuth()) return;
  JsonDocument doc;
  otaUpdateFillJson(doc.to<JsonObject>());
  String out;
  serializeJson(doc, out);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", out);
}

static void handleUpdateCheck() {
  if (!requireAuth()) return;
  if (WiFi.status() != WL_CONNECTED) {
    server.send(503, "application/json",
                "{\"ok\":false,\"error\":\"wifi indisponible\"}");
    return;
  }
  otaUpdateRequestCheck();
  server.send(202, "application/json", "{\"ok\":true}");
}

static void handleUpdateInstall() {
  if (!requireAuth()) return;
  if (!otaUpdateRequestInstall()) {
    server.send(409, "application/json",
                "{\"ok\":false,\"error\":\"aucune mise a jour disponible\"}");
    return;
  }
  server.send(202, "application/json", "{\"ok\":true}");
}

static void handleOtaDone() {
  if (!otaUploadAuthorized) {
    server.send(401, "application/json", "{\"error\":\"unauthorized\"}");
    return;
  }
  otaUploadAuthorized = false;
  bool ok = !Update.hasError();
  server.sendHeader("Connection", "close");
  server.send(200, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"echec maj\"}");
  if (ok) { delay(800); ESP.restart(); }
}
static void handleOtaUpload() {
  HTTPUpload& up = server.upload();
  if (up.status == UPLOAD_FILE_START) {
    otaUploadAuthorized = requestAuthorized();
    if (!otaUploadAuthorized) return;
    Update.begin(UPDATE_SIZE_UNKNOWN);
  }
  else if (!otaUploadAuthorized) return;
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
  webAuthBegin();
  const char *headers[] = {"X-Auth-Token"};
  server.collectHeaders(headers, 1);
  if (apMode) dns.start(DNS_PORT, "*", AP_IP);
  server.on("/",            HTTP_GET,  handleRoot);
  server.on("/icon.png",    HTTP_GET,  handleIcon);
  server.on("/manifest.webmanifest", HTTP_GET, handleManifest);
  server.on("/api/auth/status", HTTP_GET, handleAuthStatus);
  server.on("/api/auth/setup", HTTP_POST, handleAuthSetup);
  server.on("/api/auth/login", HTTP_POST, handleAuthLogin);
  server.on("/api/auth/password", HTTP_POST, handleAuthPassword);
  server.on("/api/auth/logout", HTTP_POST, handleAuthLogout);
  server.on("/api/state",   HTTP_GET,  handleState);
  server.on("/api/leds",    HTTP_GET,  handleLeds);
  server.on("/api/scan",    HTTP_GET,  handleScan);
  server.on("/api/config",  HTTP_POST, handleSaveConfig);
  server.on("/api/wifi",    HTTP_POST, handleWifi);
  server.on("/api/setup",   HTTP_POST, handleSetup);
  server.on("/api/reboot",  HTTP_POST, handleReboot);
  server.on("/api/update",  HTTP_GET,  handleUpdateStatus);
  server.on("/api/update/check", HTTP_POST, handleUpdateCheck);
  server.on("/api/update/install", HTTP_POST, handleUpdateInstall);
  server.on("/api/ota",     HTTP_POST, handleOtaDone, handleOtaUpload);
  server.on("/generate_204", handleNotFound);
  server.on("/gen_204",      handleNotFound);
  server.onNotFound(handleNotFound);
  server.begin();
  if (!apMode) otaUpdateBegin();
}

void webLoop() {
  if (s_apMode) dns.processNextRequest();
  server.handleClient();
}
