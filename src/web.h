#pragma once
#include <Arduino.h>

// Note : ce fichier s'appelle "web.h" (et non "webserver.h") pour eviter une
// collision de casse avec le header systeme <WebServer.h> sur les systemes de
// fichiers insensibles a la casse (macOS), ou <WebServer.h> resoudrait sinon
// vers ce fichier local au lieu de la librairie Arduino.

// apMode = true quand l'ESP32 sert le portail captif (pas de WiFi configure / echec).
void webBegin(bool apMode);
void webLoop();   // a appeler dans loop() : DNS captif + traitement des requetes HTTP
