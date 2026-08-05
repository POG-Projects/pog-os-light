#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <ESPmDNS.h>
#include <time.h>
#include "config.h"
#include "leds.h"
#include "display.h"
#include "web.h"
#include "pogdev.h"

// Connexion STA bas-niveau, non bloquante, compatible WPA2 et WPA2/WPA3 transition.
static void wifiStaConnect() {
  wifi_config_t wc; memset(&wc, 0, sizeof(wc));
  strlcpy((char*)wc.sta.ssid,     g_config.wifiSsid.c_str(), sizeof(wc.sta.ssid));
  strlcpy((char*)wc.sta.password, g_config.wifiPass.c_str(), sizeof(wc.sta.password));
  wc.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
  wc.sta.pmf_cfg.capable    = true;
  wc.sta.pmf_cfg.required   = false;
  wc.sta.sae_pwe_h2e        = WPA3_SAE_PWE_BOTH;
  esp_wifi_set_config(WIFI_IF_STA, &wc);
  esp_wifi_connect();
}

static void startAP() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(AP_SSID);
  Serial.printf("[AP] %s -> http://192.168.4.1\n", AP_SSID);
}

void setup() {
  Serial.begin(115200);
  delay(150);
  Serial.println("\n=== PogLight ===");

  configBegin();
  configLoad();
  ledsBegin();
  displayBegin();   // le controleur reste utilisable sans reseau sur la cible avec ecran

  // WiFi pour le controle web et l'OTA : le rendu LED ne l'attend jamais.
  if (g_config.wifiSsid.length()) {
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    WiFi.setMinSecurity(WIFI_AUTH_OPEN);
    WiFi.setHostname(MDNS_NAME);
    wifiStaConnect();
    webBegin(false);
  } else {
    startAP();
    webBegin(true);
  }
}

void loop() {
  webLoop();

  uint32_t now = millis();
  static uint32_t lastFrame = 0;
  if (now - lastFrame >= 16) { lastFrame = now; ledsLoop(); }

  // mDNS demarre des que la connexion (asynchrone) est etablie.
  static bool mdns = false;
  if (!mdns && WiFi.status() == WL_CONNECTED) {
    mdns = true;
    // L'heure UTC sert aussi de phase d'animation commune entre les lampes.
    configTime(0, 0, "pool.ntp.org", "time.cloudflare.com", "time.google.com");
    if (MDNS.begin(MDNS_NAME)) MDNS.addService("http", "tcp", 80);
    pogdevBegin();
    Serial.printf("[WiFi] OK IP: %s -> http://%s.local\n", WiFi.localIP().toString().c_str(), MDNS_NAME);
  }
}
