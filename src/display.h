#pragma once

// Ecran OLED SSD1315/SSD1306 128x64 + navigation par boutons. Les GPIO sont
// configurables, y compris sur les cibles compactes sans peripheriques montes.
extern volatile bool g_oledFound;
extern volatile int  g_oledAddr;
extern volatile bool g_oledSwapped;

void displayBegin();   // init optionnelle OLED/boutons + task UI
