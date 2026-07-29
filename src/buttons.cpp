#include "buttons.h"
#include "config.h"

// Boutons CAPACITIFS (touch) : chaque GPIO est relie a un bout de laiton, pas a la
// masse. Sur ESP32-S3, touchRead() RENVOIE UNE VALEUR PLUS ELEVEE quand on touche.
// On etablit une ligne de base (qui suit lentement la derive) et on detecte un appui
// quand la lecture depasse la base de +THRESHOLD%.
static const uint8_t  PINS[4]     = { BTN_UP_PIN, BTN_DOWN_PIN, BTN_LEFT_PIN, BTN_RIGHT_PIN };
static uint32_t       baseline[4] = { 0, 0, 0, 0 };
static bool           held[4]     = { false, false, false, false };
static uint32_t       lastEvt[4]  = { 0, 0, 0, 0 };

#define TOUCH_THRESHOLD_NUM  4        // appui si lecture > base * (1 + NUM/DEN)
#define TOUCH_THRESHOLD_DEN  10       // ici +40 %

void buttonsBegin() {
#ifdef POGLIGHT_HEADLESS
  return;
#else
  for (int i = 0; i < 4; i++) {
    uint32_t v = 0;
    for (int k = 0; k < 8; k++) { v += touchRead(PINS[i]); delay(4); }
    baseline[i] = v / 8;
    if (baseline[i] == 0) baseline[i] = 1;
  }
#endif
}

int buttonsPoll() {
#ifdef POGLIGHT_HEADLESS
  return BTN_NONE;
#else
  uint32_t now = millis();
  for (int i = 0; i < 4; i++) {
    uint32_t v = touchRead(PINS[i]);
    uint32_t thresh = baseline[i] + (baseline[i] * TOUCH_THRESHOLD_NUM) / TOUCH_THRESHOLD_DEN;
    bool isT = v > thresh;
    if (!isT) {
      baseline[i] = (baseline[i] * 15 + v) / 16;   // suit la derive quand non touche
      held[i] = false;
    } else if (!held[i] && (now - lastEvt[i]) > 220) {
      held[i] = true; lastEvt[i] = now;
      return i;                                    // front d'appui
    }
  }
  return BTN_NONE;
#endif
}
