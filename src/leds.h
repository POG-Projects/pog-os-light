#pragma once
#include <Arduino.h>

void   ledsBegin();        // init FastLED (adressable) ou PWM (analogique) selon la config
void   ledsLoop();         // rend le pattern de test courant
String ledsSnapshot();     // etat hex (logique) des LED, pour un apercu web
bool ledsEffectSyncJoin(const char *groupId, const char *leaderEntityId,
                        uint16_t presentationDelayMs,
                        int16_t calibrationOffsetMs,
                        const char *visualizer);
bool ledsEffectSyncLeave(const char *groupId);
void ledsEffectSyncCancel();
bool ledsEffectSyncFrame(uint32_t seq, uint64_t monoMs, uint64_t presentAtMs,
                         uint16_t leadMs, float level, float bass, float treble,
                         uint32_t receivedMs);
extern volatile int g_walkPos;   // position courante du pixel (TP_WALK/TP_FILL), pour l'OLED
