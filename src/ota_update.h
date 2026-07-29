#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

// Vérifie automatiquement la dernière release GitHub une fois le Wi-Fi prêt,
// puis toutes les six heures. Les téléchargements restent explicitement
// confirmés depuis l'interface web.
void otaUpdateBegin();
void otaUpdateRequestCheck();
bool otaUpdateRequestInstall();
void otaUpdateFillJson(JsonObject out);

