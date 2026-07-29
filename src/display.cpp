#include <U8g2lib.h>
#include <Wire.h>
#include "config.h"
#include "buttons.h"
#include "leds.h"
#include "display.h"

// Deux cablages I2C possibles (SDA/SCL normal ou inverse), choisi au boot par scan.
static U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8gA(U8G2_R0, U8X8_PIN_NONE, OLED_SCL_PIN, OLED_SDA_PIN);
static U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8gB(U8G2_R0, U8X8_PIN_NONE, OLED_SDA_PIN, OLED_SCL_PIN);
static U8G2* g_disp = &u8gA;
#define u8g2 (*g_disp)

volatile bool g_oledFound   = false;
volatile int  g_oledAddr    = -1;
volatile bool g_oledSwapped = false;

static const char* PAT_NAMES[TP_COUNT] = {
  "Couleur pleine", "Ordre couleurs", "Compter (defile)", "Remplissage",
  "Arc-en-ciel", "Chenillard", "Respiration", "Feu", "Scintillement",
  "Degrade", "Balayage", "Blanc plein", "Eteint"
};
static const char* ORDER_NAMES[ORDER_COUNT] = { "RGB","RBG","GRB","GBR","BRG","BGR" };
static const uint32_t QUICK_COLORS[] = {
  0x19C37D, 0xFF304F, 0xFFB000, 0x2F80FF, 0x8B5CF6, 0xFFFFFF
};

#define MAIN_COUNT (TP_COUNT + 1)        // patterns + "Reglages"
#define MI_SETTINGS TP_COUNT             // index de l'item "Reglages"

enum { ST_NUM=0, ST_PIN, ST_MODE, ST_ORDER, ST_BRIGHT, ST_CURRENT, ST_REVERSE, ST_SPEED, ST_REBOOT, ST_BACK, ST_NB };

enum { LV_MAIN=0, LV_SETTINGS=1 };
static int  s_level = LV_MAIN;
static int  s_sel = TP_RAINBOW;
static int  s_setSel = 0;
static bool s_dirty = false;

static void lock()   { xSemaphoreTake(g_configMutex, portMAX_DELAY); }
static void unlock() { xSemaphoreGive(g_configMutex); s_dirty = true; }
static void saveIfDirty() {
  if (!s_dirty) return;
  xSemaphoreTake(g_configMutex, portMAX_DELAY); configSave(); xSemaphoreGive(g_configMutex); s_dirty = false;
}

// ---------------- rendu ----------------
static void rowSel(int idx, int selIdx, int y, const char* txt, int w) {
  if (idx == selIdx) { u8g2.drawBox(0, y - 10, w, 12); u8g2.setDrawColor(0); }
  u8g2.drawStr(3, y, txt);
  u8g2.setDrawColor(1);
}

static void drawMain() {
  u8g2.setFont(u8g2_font_6x12_tf);
  int top = s_sel - 1; if (top < 0) top = 0; if (top > MAIN_COUNT - 4) top = max(0, MAIN_COUNT - 4);
  for (int r = 0; r < 4 && top + r < MAIN_COUNT; r++) {
    int idx = top + r, y = 12 + r * 13;
    const char* t = (idx == MI_SETTINGS) ? "Reglages >" : PAT_NAMES[idx];
    rowSel(idx, s_sel, y, t, 102);
  }
  u8g2.drawHLine(0, 53, 128);
  u8g2.setFont(u8g2_font_5x8_tf);
  char foot[28] = "";
  if (s_sel == TP_SOLID)         snprintf(foot, sizeof(foot), "< couleur >");
  else if (s_sel == TP_WALK || s_sel == TP_FILL) snprintf(foot, sizeof(foot), "LED %d / %d", g_walkPos + 1, g_config.numLeds);
  else if (s_sel == MI_SETTINGS) snprintf(foot, sizeof(foot), "[>] ouvrir");
  else                           snprintf(foot, sizeof(foot), "< vitesse %d >", g_config.speed);
  u8g2.drawStr(2, 62, foot);
}

static void drawSettings() {
  u8g2.setFont(u8g2_font_7x14B_tf); u8g2.drawStr(0, 12, "Reglages"); u8g2.drawHLine(0, 15, 128);
  u8g2.setFont(u8g2_font_6x12_tf);
  int top = s_setSel - 2; if (top < 0) top = 0; if (top > ST_NB - 3) top = max(0, ST_NB - 3);
  for (int r = 0; r < 3 && top + r < ST_NB; r++) {
    int idx = top + r, y = 30 + r * 13; char line[32];
    switch (idx) {
      case ST_NUM:    snprintf(line, sizeof(line), "Nb LED: %d", g_config.numLeds); break;
      case ST_PIN:    snprintf(line, sizeof(line), "Sortie: GPIO%d", g_config.ledPin); break;
      case ST_MODE:   snprintf(line, sizeof(line), "Mode: %s", g_config.analog ? "Analogique" : "Adressable"); break;
      case ST_ORDER:  snprintf(line, sizeof(line), "Ordre: %s", ORDER_NAMES[g_config.colorOrder % ORDER_COUNT]); break;
      case ST_BRIGHT: snprintf(line, sizeof(line), "Lumin.: %d", g_config.brightness); break;
      case ST_CURRENT:snprintf(line, sizeof(line), "Limite: %d mA", g_config.maxMilliAmps); break;
      case ST_REVERSE:snprintf(line, sizeof(line), "Sens: %s", g_config.reverse ? "Inverse" : "Normal"); break;
      case ST_SPEED:  snprintf(line, sizeof(line), "Vitesse: %d", g_config.speed); break;
      case ST_REBOOT: snprintf(line, sizeof(line), "Sauver + redemarrer"); break;
      case ST_BACK:   snprintf(line, sizeof(line), "< Retour"); break;
    }
    rowSel(idx, s_setSel, y, line, 128);
  }
}

static void draw() {
  u8g2.clearBuffer();
  if (s_level == LV_MAIN) drawMain(); else drawSettings();
  u8g2.sendBuffer();
}

// ---------------- navigation ----------------
static void mainButton(int e) {
  if (e == BTN_UP || e == BTN_DOWN) {
    s_sel = (s_sel + (e == BTN_DOWN ? 1 : MAIN_COUNT - 1)) % MAIN_COUNT;
    if (s_sel < TP_COUNT) { lock(); g_config.pattern = s_sel; unlock(); }
  } else if (e == BTN_RIGHT) {
    if (s_sel == MI_SETTINGS) { s_level = LV_SETTINGS; s_setSel = 0; }
    else if (s_sel == TP_SOLID) {
      lock();
      int idx = 0;
      for (int i = 0; i < 6; i++) if (g_config.primaryColor == QUICK_COLORS[i]) idx = i;
      g_config.primaryColor = QUICK_COLORS[(idx + 1) % 6];
      unlock();
    }
    else { lock(); if (g_config.speed <= 95) g_config.speed += 5; unlock(); }
  } else if (e == BTN_LEFT) {
    if (s_sel == TP_SOLID) {
      lock();
      int idx = 0;
      for (int i = 0; i < 6; i++) if (g_config.primaryColor == QUICK_COLORS[i]) idx = i;
      g_config.primaryColor = QUICK_COLORS[(idx + 5) % 6];
      unlock();
    }
    else if (s_sel != MI_SETTINGS) { lock(); if (g_config.speed >= 5) g_config.speed -= 5; unlock(); }
  }
}

static void settingsButton(int e) {
  if (e == BTN_UP || e == BTN_DOWN) {
    s_setSel = (s_setSel + (e == BTN_DOWN ? 1 : ST_NB - 1)) % ST_NB;
    return;
  }
  int d = (e == BTN_RIGHT) ? 1 : -1;
  switch (s_setSel) {
    case ST_NUM:    lock(); g_config.numLeds = constrain((int)g_config.numLeds + d * 5, 1, MAX_LEDS); unlock(); break;
    case ST_PIN:    lock(); g_config.ledPin = (g_config.ledPin == 18) ? 16 : 18; unlock(); break;
    case ST_MODE:   lock(); g_config.analog = !g_config.analog; unlock(); break;
    case ST_ORDER:  lock(); g_config.colorOrder = (g_config.colorOrder + ORDER_COUNT + d) % ORDER_COUNT; unlock(); break;
    case ST_BRIGHT: lock(); g_config.brightness = constrain((int)g_config.brightness + d * 10, 5, 255); unlock(); break;
    case ST_CURRENT:lock(); g_config.maxMilliAmps = constrain((int)g_config.maxMilliAmps + d * 100, 100, 10000); unlock(); break;
    case ST_REVERSE:lock(); g_config.reverse = !g_config.reverse; unlock(); break;
    case ST_SPEED:  lock(); g_config.speed = constrain((int)g_config.speed + d * 5, 0, 100); unlock(); break;
    case ST_REBOOT: if (e == BTN_RIGHT) { saveIfDirty(); delay(200); ESP.restart(); } break;
    case ST_BACK:   if (e == BTN_RIGHT || e == BTN_LEFT) { saveIfDirty(); s_level = LV_MAIN; } break;
  }
}

static void uiTask(void* arg) {
  vTaskDelay(pdMS_TO_TICKS(700));   // laisse le splash
  uint32_t lastDraw = 0, lastSave = 0;
  for (;;) {
    int e = buttonsPoll();
    if (e != BTN_NONE) { (s_level == LV_MAIN) ? mainButton(e) : settingsButton(e); lastDraw = 0; }
    uint32_t now = millis();
    if (now - lastDraw > 120) { lastDraw = now; draw(); }
    if (s_dirty && now - lastSave > 4000) { lastSave = now; saveIfDirty(); }   // persiste sans user
    vTaskDelay(pdMS_TO_TICKS(15));
  }
}

// Scan I2C COMPLET 1->127 (comme PogLight, qui detectait l'ecran). Scanner toute la
// plage "reveille" le bus avant 0x3C : la toute 1ere transaction apres Wire.begin
// NAK souvent, ce qui faisait echouer un test direct de 0x3C.
static int i2cFind(int sda, int scl) {
  Wire.begin(sda, scl); delay(30);
  int f = -1;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) { f = a; break; }
  }
  Wire.end();
  return f;
}

void displayBegin() {
#ifdef POGLIGHT_HEADLESS
  return;
#else
  buttonsBegin();
  // L'OLED peut mettre ~1s a repondre apres mise sous tension : on ressaie pendant ~3s
  // (PogLight marchait car ledsTest() introduisait ce delai avant le scan I2C).
  int a = -1; bool swap = false;
  for (int t = 0; t < 12 && a < 0; t++) {
    a = i2cFind(OLED_SDA_PIN, OLED_SCL_PIN); if (a >= 0) { swap = false; break; }   // SDA=13/SCL=11
    a = i2cFind(OLED_SCL_PIN, OLED_SDA_PIN); if (a >= 0) { swap = true;  break; }   // inverse
    delay(200);
  }
  g_oledFound   = (a >= 0);
  g_oledAddr    = a;
  g_oledSwapped = (a >= 0) ? swap : g_config.oledSwap;   // pas trouve -> orientation manuelle (reglage)
  g_disp = g_oledSwapped ? &u8gB : &u8gA;

  u8g2.setI2CAddress((a >= 0 ? a : OLED_ADDR) * 2);
  u8g2.begin();
  u8g2.setBusClock(400000);
  u8g2.setContrast(255);
  // Splash plein ecran : si le panneau fonctionne, il s'allume entierement ~0.8s.
  u8g2.clearBuffer(); u8g2.drawBox(0, 0, 128, 64); u8g2.sendBuffer();
  delay(800);
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14B_tf); u8g2.drawStr(8, 30, "PogLight");
  u8g2.setFont(u8g2_font_5x8_tf);   u8g2.drawStr(8, 46, "controleur LED");
  u8g2.sendBuffer();
  s_sel = g_config.pattern;
  xTaskCreatePinnedToCore(uiTask, "ui", 6144, nullptr, 1, nullptr, 0);
#endif
}
