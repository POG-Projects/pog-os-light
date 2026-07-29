#pragma once
#include <Arduino.h>

void   ledsBegin();        // init FastLED (adressable) ou PWM (analogique) selon la config
void   ledsLoop();         // rend le pattern de test courant
String ledsSnapshot();     // etat hex (logique) des LED, pour un apercu web
extern volatile int g_walkPos;   // position courante du pixel (TP_WALK/TP_FILL), pour l'OLED
