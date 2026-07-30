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
static void pSolid(uint16_t start, uint16_t count, const CRGB& c) {
  fill_solid(work + start, count, c);
}

static void pWalk(uint16_t start, uint16_t count, uint32_t now, uint16_t stepMs, bool fill) {
  int span = count;
  int pos = (now / (stepMs ? stepMs : 1)) % span;
  g_walkPos = start + pos;
  fill_solid(work + start, count, CRGB::Black);
  if (fill) {
    for (int i = 0; i <= pos; i++) work[start + i] = CRGB::White;
  } else {
    work[start + pos] = CRGB::White;
    if (pos + 1 < span) work[start + pos + 1] = CRGB(40,40,40);
  }
}

static uint16_t speedInterval(uint8_t speed, uint16_t slowMs, uint16_t fastMs) {
  return (uint16_t)map(speed, 0, 100, slowMs, fastMs);
}

static uint8_t breatheLevel(uint32_t now, uint16_t cycleMs) {
  uint8_t phase = (uint8_t)(((uint64_t)now * 256ULL) / max((uint16_t)1, cycleMs));
  return qadd8(10, scale8(sin8(phase), 245));
}

static void pRainbow(uint16_t start, uint16_t count, uint32_t now, uint16_t stepMs) {
  fill_rainbow(work + start, count,
               (uint8_t)(now / max((uint16_t)1, stepMs)),
               max(1, 255 / count));
}

static void pChase(uint16_t start, uint16_t count, uint32_t now,
                   uint16_t stepMs, const CRGB& c) {
  fill_solid(work + start, count, CRGB::Black);
  uint8_t step = (now / max((uint16_t)1, stepMs)) % 3;
  for (int i = 0; i < count; i++) if ((i % 3) == step) work[start + i] = c;
}

static void pBreathe(uint16_t start, uint16_t count, uint32_t now,
                     uint16_t cycleMs, const CRGB& c) {
  uint8_t b = breatheLevel(now, cycleMs);
  CRGB x = c; x.nscale8_video(b);
  fill_solid(work + start, count, x);
}

static void pFire(uint16_t start, uint16_t count, uint32_t now, uint16_t stepMs) {
  static byte heat[MAX_LEDS];
  static uint32_t lastTick[MAX_LEDS];
  uint32_t tick = now / max((uint16_t)1, stepMs);
  if (lastTick[start] != tick) {
    lastTick[start] = tick;
    for (int i = 0; i < count; i++) {
      int at = start + i;
      heat[at] = qsub8(heat[at], random8(0, ((55 * 10) / count) + 2));
    }
    for (int k = count - 1; k >= 2; k--) {
      heat[start + k] = (heat[start + k - 1] + heat[start + k - 2] + heat[start + k - 2]) / 3;
    }
    if (random8() < 120) {
      int y = start + random8(min(7, (int)count));
      heat[y] = qadd8(heat[y], random8(160, 255));
    }
  }
  for (int i = 0; i < count; i++) work[start + i] = HeatColor(heat[start + i]);
}

static CRGB fromRgb(uint32_t rgb) {
  return CRGB((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

static void pTwinkle(uint16_t start, uint16_t count, uint32_t now,
                     uint16_t stepMs, const CRGB& color) {
  static CRGB stars[MAX_LEDS];
  static uint32_t lastTick[MAX_LEDS];
  uint32_t tick = now / max((uint16_t)1, stepMs);
  if (lastTick[start] != tick) {
    lastTick[start] = tick;
    fadeToBlackBy(stars + start, count, 28);
    if (random8() < 150) stars[start + random16(count)] += color;
  }
  for (uint16_t i = 0; i < count; ++i) work[start + i] = stars[start + i];
}

static void pGradient(uint16_t start, uint16_t count, const CRGB& a, const CRGB& b) {
  if (count == 1) { work[start] = a; return; }
  for (int i = 0; i < count; i++) {
    work[start + i] = blend(a, b, (uint8_t)((i * 255UL) / (count - 1)));
  }
}

static void pWipe(uint16_t start, uint16_t count, uint32_t now, uint16_t stepMs,
                  const CRGB& a, const CRGB& b) {
  int pos = (now / max((uint16_t)1, stepMs)) % (count + 1);
  for (int i = 0; i < count; i++) work[start + i] = (i < pos) ? a : b;
}

static void renderPattern(uint16_t start, uint16_t count, uint8_t pattern,
                          uint8_t speed, uint32_t primaryRgb,
                          uint32_t secondaryRgb, uint32_t now) {
  if (!count) return;
  uint16_t stepMs = speedInterval(speed, 360, 12);
  CRGB primary = fromRgb(primaryRgb);
  CRGB secondary = fromRgb(secondaryRgb);
  static const CRGB ORDER_COLORS[3] = { CRGB::Red, CRGB::Green, CRGB::Blue };
  switch (pattern) {
    case TP_SOLID:   pSolid(start, count, primary); break;
    case TP_ORDER:   pSolid(start, count, ORDER_COLORS[
                          (now / speedInterval(speed, 2400, 250)) % 3]); break;
    case TP_WALK:    pWalk(start, count, now, stepMs, false); break;
    case TP_FILL:    pWalk(start, count, now, stepMs, true); break;
    case TP_RAINBOW: pRainbow(start, count, now, speedInterval(speed, 90, 3)); break;
    case TP_CHASE:   pChase(start, count, now, speedInterval(speed, 650, 35), primary); break;
    case TP_BREATHE: pBreathe(start, count, now, speedInterval(speed, 8000, 700), primary); break;
    case TP_FIRE:    pFire(start, count, now, speedInterval(speed, 180, 16)); break;
    case TP_TWINKLE: pTwinkle(start, count, now, speedInterval(speed, 220, 18), primary); break;
    case TP_GRADIENT:pGradient(start, count, primary, secondary); break;
    case TP_WIPE:    pWipe(start, count, now, stepMs, primary, secondary); break;
    case TP_WHITE:   pSolid(start, count, CRGB::White); break;
    case TP_OFF:
    default:         fill_solid(work + start, count, CRGB::Black); break;
  }
}

void ledsLoop() {
  Config snapshot;
  if (xSemaphoreTake(g_configMutex, 0) != pdTRUE) return;
  snapshot = g_config;
  xSemaphoreGive(g_configMutex);

  uint32_t now = millis();

  // --- Mode analogique (mono PWM) ---
  if (snapshot.analog) {
    uint8_t duty = 0;
    switch (snapshot.pattern) {
      case TP_OFF:     duty = 0; break;
      case TP_BREATHE:
        duty = scale8(breatheLevel(now, speedInterval(snapshot.speed, 8000, 700)),
                      snapshot.brightness);
        break;
      case TP_CHASE:
      case TP_WALK:
        duty = ((now / speedInterval(snapshot.speed, 900, 60)) % 2)
                   ? snapshot.brightness : 0;
        break;
      default:         duty = snapshot.brightness; break;
    }
    ledcWrite(snapshot.ledPin, duty);
    return;
  }

  // --- Mode adressable ---
  if (!snapshot.sectionCount) {
    renderPattern(0, g_num, snapshot.pattern, snapshot.speed,
                  snapshot.primaryColor, snapshot.secondaryColor, now);
  } else {
    fill_solid(work, g_num, CRGB::Black);
    if (snapshot.pattern != TP_OFF) {
      for (uint8_t i = 0; i < snapshot.sectionCount; ++i) {
        const LedSection& section = snapshot.sections[i];
        uint16_t start = min(section.start, (uint16_t)(g_num - 1));
        uint16_t count = min(section.count, (uint16_t)(g_num - start));
        if (!section.enabled || !section.on) continue;
        renderPattern(start, count, section.pattern, section.speed,
                      section.primaryColor, section.secondaryColor, now);
        if (section.brightness < 255) {
          for (uint16_t pixel = start; pixel < start + count; ++pixel) {
            work[pixel].nscale8_video(section.brightness);
          }
        }
      }
    }
  }
  showAddressable(snapshot.brightness, snapshot.maxMilliAmps, snapshot.reverse);
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
