#pragma once

// Ecran OLED SSD1315/SSD1306 128x64 + navigation par boutons (ESP32-S3).
// Sur la cible ESP32-C3 headless, displayBegin() est un no-op.
extern volatile bool g_oledFound;
extern volatile int  g_oledAddr;
extern volatile bool g_oledSwapped;

void displayBegin();   // init OLED + boutons + lance la task UI (coeur 0)
