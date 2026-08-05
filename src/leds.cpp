#include <FastLED.h>
#include <sys/time.h>
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

static uint32_t animationClockMs() {
  timeval tv{};
  gettimeofday(&tv, nullptr);
  if (tv.tv_sec > 1700000000) {
    return (uint32_t)((uint64_t)tv.tv_sec * 1000ULL + tv.tv_usec / 1000ULL);
  }
  return millis();
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
  uint16_t t = (uint16_t)(now / max((uint16_t)1, stepMs));
  for (uint16_t i = 0; i < count; ++i) {
    uint8_t noise = inoise8(i * 58, t * 23);
    uint8_t cooling = (uint8_t)((i * 105UL) / max((uint16_t)1, count));
    work[start + i] = HeatColor(qsub8(noise, cooling));
  }
}

static CRGB fromRgb(uint32_t rgb) {
  return CRGB((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

static void pTwinkle(uint16_t start, uint16_t count, uint32_t now,
                     uint16_t stepMs, const CRGB& color) {
  uint32_t interval = max((uint16_t)1, stepMs) * 12UL;
  uint32_t slot = now / interval;
  uint8_t phase = (uint8_t)(((now % interval) * 255UL) / interval);
  for (uint16_t i = 0; i < count; ++i) {
    uint16_t seed = (uint16_t)(i * 2053U + slot * 1381U + start * 97U);
    seed ^= seed << 7;
    seed ^= seed >> 9;
    seed ^= seed << 8;
    uint8_t level = (seed & 0x07) == 0 ? sin8(phase) : 0;
    work[start + i] = color;
    work[start + i].nscale8_video(level);
  }
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

static void pPaletteNoise(uint16_t start, uint16_t count, uint32_t now,
                          uint16_t stepMs, const CRGBPalette16& palette,
                          uint8_t spatialScale) {
  uint16_t t = (uint16_t)(now / max((uint16_t)1, stepMs));
  for (uint16_t i = 0; i < count; ++i) {
    uint8_t n = inoise8(i * spatialScale, t * 7);
    work[start + i] = ColorFromPalette(palette, n, 210, LINEARBLEND);
  }
}

static void pAurora(uint16_t start, uint16_t count, uint32_t now, uint16_t stepMs) {
  uint16_t t = (uint16_t)(now / max((uint16_t)1, stepMs));
  for (uint16_t i = 0; i < count; ++i) {
    uint8_t n = inoise8(i * 31, t * 5);
    uint8_t hue = 82 + scale8(n, 78);
    work[start + i] = CHSV(hue, 210, qadd8(55, scale8(n, 195)));
  }
}

static void pComet(uint16_t start, uint16_t count, uint32_t now,
                   uint16_t stepMs, const CRGB& color) {
  fill_solid(work + start, count, CRGB::Black);
  uint16_t span = max((uint16_t)1, (uint16_t)(count * 2 - 2));
  uint16_t phase = (now / max((uint16_t)1, stepMs)) % span;
  int head = phase < count ? phase : span - phase;
  uint16_t tail = min((uint16_t)12, count);
  for (uint16_t d = 0; d < tail; ++d) {
    int at = head - d;
    if (at < 0) break;
    work[start + at] = color;
    work[start + at].nscale8_video(255 - (d * 230 / tail));
  }
}

static void pWave(uint16_t start, uint16_t count, uint32_t now,
                  uint16_t stepMs, const CRGB& a, const CRGB& b) {
  uint8_t phase = (uint8_t)(now / max((uint16_t)1, stepMs));
  for (uint16_t i = 0; i < count; ++i) {
    work[start + i] = blend(a, b, sin8(phase + i * 18));
  }
}

static void pCandle(uint16_t start, uint16_t count, uint32_t now, uint16_t stepMs) {
  uint16_t t = (uint16_t)(now / max((uint16_t)1, stepMs));
  for (uint16_t i = 0; i < count; ++i) {
    uint8_t flicker = inoise8(i * 71, t * 29);
    work[start + i] = CRGB(255, 92 + scale8(flicker, 76), scale8(flicker, 22));
    work[start + i].nscale8_video(165 + scale8(flicker, 90));
  }
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
    case TP_OFF:     fill_solid(work + start, count, CRGB::Black); break;
    case TP_AURORA:  pAurora(start, count, now, speedInterval(speed, 150, 10)); break;
    case TP_OCEAN:   pPaletteNoise(start, count, now, speedInterval(speed, 150, 8),
                                  OceanColors_p, 37); break;
    case TP_LAVA:    pPaletteNoise(start, count, now, speedInterval(speed, 170, 9),
                                  LavaColors_p, 44); break;
    case TP_COMET:   pComet(start, count, now, speedInterval(speed, 130, 8), primary); break;
    case TP_WAVE:    pWave(start, count, now, speedInterval(speed, 90, 3),
                          primary, secondary); break;
    case TP_CANDLE:  pCandle(start, count, now, speedInterval(speed, 180, 18)); break;
    default:         fill_solid(work + start, count, CRGB::Black); break;
  }
}

void ledsLoop() {
  Config snapshot;
  if (xSemaphoreTake(g_configMutex, 0) != pdTRUE) return;
  snapshot = g_config;
  xSemaphoreGive(g_configMutex);

  uint32_t now = animationClockMs();

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
