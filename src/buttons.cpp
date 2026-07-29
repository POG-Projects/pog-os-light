#include "buttons.h"
#include "config.h"
#include <soc/soc_caps.h>

// Les quatre commandes peuvent etre des poussoirs vers GND (INPUT_PULLUP) ou
// des electrodes tactiles sur les puces qui possedent le peripherique touch.
static uint8_t        pins[4]     = { BTN_UP_PIN, BTN_DOWN_PIN, BTN_LEFT_PIN, BTN_RIGHT_PIN };
static uint32_t       baseline[4] = { 0, 0, 0, 0 };
static bool           held[4]     = { false, false, false, false };
static uint32_t       lastEvt[4]  = { 0, 0, 0, 0 };

#define TOUCH_THRESHOLD_NUM  3
#define TOUCH_THRESHOLD_DEN  10

void buttonsBegin() {
  if (!g_config.buttonsEnabled) return;
  for (int i = 0; i < 4; i++) {
    pins[i] = g_config.buttonPins[i];
    held[i] = false;
    lastEvt[i] = 0;
  }
  if (g_config.buttonMode == BIM_DIGITAL_PULLUP) {
    for (uint8_t pin : pins) pinMode(pin, INPUT_PULLUP);
    return;
  }
#if SOC_TOUCH_SENSOR_SUPPORTED
  for (int i = 0; i < 4; i++) {
    uint32_t v = 0;
    for (int k = 0; k < 8; k++) { v += touchRead(pins[i]); delay(4); }
    baseline[i] = v / 8;
    if (baseline[i] == 0) baseline[i] = 1;
  }
#endif
}

int buttonsPoll() {
  if (!g_config.buttonsEnabled) return BTN_NONE;
  uint32_t now = millis();
  if (g_config.buttonMode == BIM_DIGITAL_PULLUP) {
    for (int i = 0; i < 4; ++i) {
      bool pressed = digitalRead(pins[i]) == LOW;
      if (!pressed) {
        held[i] = false;
      } else if (!held[i] && now - lastEvt[i] > 60) {
        held[i] = true;
        lastEvt[i] = now;
        return i;
      }
    }
    return BTN_NONE;
  }
#if SOC_TOUCH_SENSOR_SUPPORTED
  for (int i = 0; i < 4; i++) {
    uint32_t v = touchRead(pins[i]);
    uint32_t delta = v > baseline[i] ? v - baseline[i] : baseline[i] - v;
    uint32_t threshold = max((uint32_t)2, (baseline[i] * TOUCH_THRESHOLD_NUM) / TOUCH_THRESHOLD_DEN);
    bool isT = delta > threshold;
    if (!isT) {
      baseline[i] = (baseline[i] * 15 + v) / 16;   // suit la derive quand non touche
      held[i] = false;
    } else if (!held[i] && (now - lastEvt[i]) > 220) {
      held[i] = true; lastEvt[i] = now;
      return i;                                    // front d'appui
    }
  }
#endif
  return BTN_NONE;
}
