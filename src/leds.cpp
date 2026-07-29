#include <FastLED.h>
#include "config.h"
#include "leds.h"

volatile int g_walkPos = 0;

static CRGB    work[MAX_LEDS];     // tampon logique (R,V,B)
static CRGB    out[MAX_LEDS];      // tampon envoye a FastLED (apres permutation d'ordre)
static uint16_t g_num = 60;
static bool    g_analog = false;

// FastLED en RGB fixe : l'ordre reel est applique en logiciel (permutation).
static void addLedsForPin(uint8_t pin, uint16_t count) {
  switch (pin) {
#if CONFIG_IDF_TARGET_ESP32C3
    case 3:  FastLED.addLeds<LED_TYPE, 3,  RGB>(out, count); break;
    case 4:  FastLED.addLeds<LED_TYPE, 4,  RGB>(out, count); break;
    case 5:  FastLED.addLeds<LED_TYPE, 5,  RGB>(out, count); break;
    case 6:  FastLED.addLeds<LED_TYPE, 6,  RGB>(out, count); break;
    case 7:  FastLED.addLeds<LED_TYPE, 7,  RGB>(out, count); break;
    case 10: FastLED.addLeds<LED_TYPE, 10, RGB>(out, count); break;
    case 2:
    default: FastLED.addLeds<LED_TYPE, 2,  RGB>(out, count); break;
#elif CONFIG_IDF_TARGET_ESP32S3
    case 2:  FastLED.addLeds<LED_TYPE, 2,  RGB>(out, count); break;
    case 15: FastLED.addLeds<LED_TYPE, 15, RGB>(out, count); break;
    case 16: FastLED.addLeds<LED_TYPE, 16, RGB>(out, count); break;
    case 17: FastLED.addLeds<LED_TYPE, 17, RGB>(out, count); break;
    case 21: FastLED.addLeds<LED_TYPE, 21, RGB>(out, count); break;
    case 38: FastLED.addLeds<LED_TYPE, 38, RGB>(out, count); break;
    case 47: FastLED.addLeds<LED_TYPE, 47, RGB>(out, count); break;
    case 48: FastLED.addLeds<LED_TYPE, 48, RGB>(out, count); break;
    case 18:
    default: FastLED.addLeds<LED_TYPE, 18, RGB>(out, count); break;
#else
    case 2:  FastLED.addLeds<LED_TYPE, 2,  RGB>(out, count); break;
    case 4:  FastLED.addLeds<LED_TYPE, 4,  RGB>(out, count); break;
    case 5:  FastLED.addLeds<LED_TYPE, 5,  RGB>(out, count); break;
    case 12: FastLED.addLeds<LED_TYPE, 12, RGB>(out, count); break;
    case 13: FastLED.addLeds<LED_TYPE, 13, RGB>(out, count); break;
    case 14: FastLED.addLeds<LED_TYPE, 14, RGB>(out, count); break;
    case 16: FastLED.addLeds<LED_TYPE, 16, RGB>(out, count); break;
    case 17: FastLED.addLeds<LED_TYPE, 17, RGB>(out, count); break;
    case 19: FastLED.addLeds<LED_TYPE, 19, RGB>(out, count); break;
    case 21: FastLED.addLeds<LED_TYPE, 21, RGB>(out, count); break;
    case 22: FastLED.addLeds<LED_TYPE, 22, RGB>(out, count); break;
    case 23: FastLED.addLeds<LED_TYPE, 23, RGB>(out, count); break;
    case 18:
    default: FastLED.addLeds<LED_TYPE, 18, RGB>(out, count); break;
#endif
  }
}

// Sequence des canaux logiques (0=R,1=V,2=B) pour byte0,1,2 selon l'ordre.
//   0=RGB 1=RBG 2=GRB 3=GBR 4=BRG 5=BGR
static const uint8_t ORDER_SEQ[ORDER_COUNT][3] = {
  {0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}
};

void ledsBegin() {
  g_num   = constrain((int)g_config.numLeds, 1, MAX_LEDS);
  g_analog = g_config.analog;
  if (g_analog) {
    // Mode mono/analogique : PWM sur la broche (un seul canal de luminosite).
    ledcAttach(g_config.ledPin, 5000, 8);
  } else {
    addLedsForPin(g_config.ledPin, g_num);
    FastLED.setCorrection(TypicalLEDStrip);
  }
}

// Permute work[] (logique) -> out[] selon l'ordre, applique la luminosite, envoie.
static void showAddressable(uint8_t bright, uint16_t maxMilliAmps, bool reverse) {
  const uint8_t* s = ORDER_SEQ[g_config.colorOrder % ORDER_COUNT];
  for (int i = 0; i < g_num; i++) {
    const CRGB& pixel = work[reverse ? g_num - 1 - i : i];
    uint8_t ch[3] = { pixel.r, pixel.g, pixel.b };
    out[i].raw[0] = ch[s[0]];
    out[i].raw[1] = ch[s[1]];
    out[i].raw[2] = ch[s[2]];
  }
  FastLED.setMaxPowerInVoltsAndMilliamps(5, maxMilliAmps);
  FastLED.setBrightness(bright);
  FastLED.show();
}

// ---- patterns (rendent dans work[], en RGB logique) ----
static void pSolid(const CRGB& c)      { fill_solid(work, g_num, c); }

static void pWalk(uint32_t now, uint16_t stepMs, bool fill) {
  int span = g_num;
  int pos = (now / (stepMs ? stepMs : 1)) % span;
  g_walkPos = pos;
  fill_solid(work, g_num, CRGB::Black);
  if (fill) { for (int i = 0; i <= pos; i++) work[i] = CRGB::White; }
  else      { work[pos] = CRGB::White; if (pos + 1 < g_num) work[pos + 1] = CRGB(40,40,40); }
}

static void pRainbow(uint32_t now) { fill_rainbow(work, g_num, (uint8_t)(now / 20), max(1, 255 / g_num)); }

static void pChase(uint32_t now, const CRGB& c) {
  fill_solid(work, g_num, CRGB::Black);
  uint8_t step = (now / 90) % 3;
  for (int i = 0; i < g_num; i++) if ((i % 3) == step) work[i] = c;
}

static void pBreathe(const CRGB& c) {
  uint8_t b = beatsin8(15, 10, 255);
  CRGB x = c; x.nscale8_video(b);
  fill_solid(work, g_num, x);
}

static void pFire() {
  static byte heat[MAX_LEDS];
  for (int i = 0; i < g_num; i++) heat[i] = qsub8(heat[i], random8(0, ((55 * 10) / g_num) + 2));
  for (int k = g_num - 1; k >= 2; k--) heat[k] = (heat[k-1] + heat[k-2] + heat[k-2]) / 3;
  if (random8() < 120) { int y = random8(min(7, (int)g_num)); heat[y] = qadd8(heat[y], random8(160, 255)); }
  for (int i = 0; i < g_num; i++) work[i] = HeatColor(heat[i]);
}

static CRGB fromRgb(uint32_t rgb) {
  return CRGB((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

static void pTwinkle(const CRGB& color) {
  fadeToBlackBy(work, g_num, 28);
  if (random8() < 150) work[random16(g_num)] += color;
}

static void pGradient(const CRGB& a, const CRGB& b) {
  if (g_num == 1) { work[0] = a; return; }
  for (int i = 0; i < g_num; i++) work[i] = blend(a, b, (uint8_t)((i * 255UL) / (g_num - 1)));
}

static void pWipe(uint32_t now, uint16_t stepMs, const CRGB& a, const CRGB& b) {
  int pos = (now / max((uint16_t)1, stepMs)) % (g_num + 1);
  for (int i = 0; i < g_num; i++) work[i] = (i < pos) ? a : b;
}

void ledsLoop() {
  uint8_t pat, bright, speed; uint16_t maxMilliAmps; uint32_t primaryRgb, secondaryRgb;
  bool analog, reverse;
  if (xSemaphoreTake(g_configMutex, 0) != pdTRUE) return;
  pat = g_config.pattern; bright = g_config.brightness; speed = g_config.speed;
  maxMilliAmps = g_config.maxMilliAmps; analog = g_config.analog; reverse = g_config.reverse;
  primaryRgb = g_config.primaryColor; secondaryRgb = g_config.secondaryColor;
  xSemaphoreGive(g_configMutex);

  uint32_t now = millis();
  uint16_t stepMs = map(speed, 0, 100, 300, 10);   // vitesse -> ms par pas
  CRGB primary = fromRgb(primaryRgb), secondary = fromRgb(secondaryRgb);

  // --- Mode analogique (mono PWM) ---
  if (analog) {
    uint8_t duty = 0;
    switch (pat) {
      case TP_OFF:     duty = 0; break;
      case TP_BREATHE: duty = scale8(beatsin8(15, 10, 255), bright); break;
      case TP_CHASE:
      case TP_WALK:    duty = ((now / 400) % 2) ? bright : 0; break;   // clignotement
      default:         duty = bright; break;                          // allume
    }
    ledcWrite(g_config.ledPin, duty);
    return;
  }

  // --- Mode adressable ---
  static const CRGB ORDER_COLORS[3] = { CRGB::Red, CRGB::Green, CRGB::Blue };
  switch (pat) {
    case TP_SOLID:   pSolid(primary); break;
    case TP_ORDER: { int k = (now / 1500) % 3; pSolid(ORDER_COLORS[k]); break; }  // R->V->B
    case TP_WALK:    pWalk(now, stepMs, false); break;
    case TP_FILL:    pWalk(now, stepMs, true);  break;
    case TP_RAINBOW: pRainbow(now); break;
    case TP_CHASE:   pChase(now, primary); break;
    case TP_BREATHE: pBreathe(primary); break;
    case TP_FIRE:    pFire(); break;
    case TP_TWINKLE: pTwinkle(primary); break;
    case TP_GRADIENT:pGradient(primary, secondary); break;
    case TP_WIPE:    pWipe(now, stepMs, primary, secondary); break;
    case TP_WHITE:   pSolid(CRGB::White); break;
    case TP_OFF:
    default:         fill_solid(work, g_num, CRGB::Black); break;
  }
  showAddressable(bright, maxMilliAmps, reverse);
}

String ledsSnapshot() {
  String s; s.reserve(g_num * 6 + 1);
  char b[7];
  int n = min((int)g_num, 64);   // limite l'apercu web
  for (int i = 0; i < n; i++) {
    snprintf(b, sizeof(b), "%02x%02x%02x", work[i].r, work[i].g, work[i].b);
    s += b;
  }
  return s;
}
