#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <soc/soc_caps.h>

#ifndef SOC_TOUCH_SENSOR_SUPPORTED
#define SOC_TOUCH_SENSOR_SUPPORTED 0
#endif

// ===========================================================================
// PogLight - controleur autonome de bandes LED (ARGB clockless + mono/PWM).
// L'ESP32-S3 peut utiliser un OLED et 4 boutons tactiles. La cible ESP32-C3
// est volontairement headless et se pilote depuis l'interface web.
// ===========================================================================

// Sorties LED (cablage final). FastLED exige la broche en constante de compil :
// on compile une liste de broches et on choisit au boot (voir leds.cpp).
#ifndef DEFAULT_LED_PIN
#define DEFAULT_LED_PIN  18           // sortie 1 ; 16 = sortie 2
#endif
#define MAX_LEDS         300          // capacite buffer (RAM = MAX_LEDS*3 octets)
#define MAX_SECTIONS     8            // zones logiques publiees individuellement dans PogHome
#define LED_TYPE         WS2812B      // famille clockless 800kHz (WS2811/12/13/15, SK6812)
// NB : l'ordre des couleurs est gere EN LOGICIEL (permutation), pas par FastLED.

// Peripheriques (cablage fixe).
#define BTN_UP_PIN     5
#define BTN_DOWN_PIN   6
#define BTN_LEFT_PIN   7
#define BTN_RIGHT_PIN  4
#define OLED_SDA_PIN   13
#define OLED_SCL_PIN   11
#define OLED_ADDR      0x3C

enum ButtonInputMode : uint8_t {
  BIM_DIGITAL_PULLUP = 0,  // bouton entre GPIO et GND
  BIM_CAPACITIVE,           // electrode tactile, cartes compatibles seulement
  BIM_COUNT
};

#define AP_SSID        "PogLight-Setup"
#define MDNS_NAME      "poglight"

// Effets et outils de diagnostic.
enum TestPattern : uint8_t {
  TP_SOLID = 0,
  TP_ORDER,       // identification de l'ordre des couleurs (R puis V puis B annonces)
  TP_WALK,        // un pixel qui defile (compter les LED / reperer coupures)
  TP_FILL,        // remplissage progressif
  TP_RAINBOW,     // arc-en-ciel
  TP_CHASE,       // chenillard
  TP_BREATHE,     // respiration
  TP_FIRE,        // feu
  TP_TWINKLE,     // scintillement sur la couleur principale
  TP_GRADIENT,    // degrade entre les deux couleurs
  TP_WIPE,        // balayage progressif
  TP_WHITE,       // blanc plein (test de charge / alim)
  TP_OFF,         // eteint
  TP_COUNT
};

// Ordres de couleur testables (index). Geres par permutation logicielle.
//   0=RGB 1=RBG 2=GRB 3=GBR 4=BRG 5=BGR
#define ORDER_COUNT 6

enum LightPurpose : uint8_t {
  LP_AMBIENT = 0,
  LP_TASK,
  LP_NIGHT,
  LP_PATH,
  LP_TV,
  LP_STATUS,
  LP_WAKE,
  LP_DECORATIVE,
  LP_COUNT
};

struct LedSection {
  uint16_t id        = 0;             // identite stable pour PogHome
  String   name      = "Section";
  uint16_t start     = 0;             // index physique, base zero
  uint16_t count     = 1;
  bool     enabled   = true;
  bool     on        = true;
  uint8_t  purpose   = LP_AMBIENT;
  uint8_t  brightness = 255;
  uint8_t  pattern   = TP_SOLID;
  uint32_t primaryColor   = 0x19C37D;
  uint32_t secondaryColor = 0x6C3BFF;
  uint8_t  speed      = 50;
};

struct Config {
  // Materiel / bande controlee
  uint8_t  ledPin     = DEFAULT_LED_PIN;
  uint16_t numLeds    = 60;
  uint8_t  brightness = 100;          // 0-255
  uint16_t maxMilliAmps = 2000;        // limite logicielle de puissance a 5 V
  uint8_t  colorOrder = 2;            // defaut GRB (le plus courant)
  bool     analog     = false;        // mode mono/analogique (PWM) au lieu d'adressable
  bool     reverse    = false;         // inverse le sens logique de la bande
  bool     oledSwap   = true;         // migration des anciennes configurations uniquement
#if CONFIG_IDF_TARGET_ESP32C3
  bool     oledEnabled = false;
  bool     buttonsEnabled = false;
  uint8_t  buttonMode = BIM_DIGITAL_PULLUP;
  uint8_t  oledSda    = 5;
  uint8_t  oledScl    = 6;
#else
  bool     oledEnabled = true;
  bool     buttonsEnabled = true;
  uint8_t  buttonMode = BIM_CAPACITIVE;
  uint8_t  oledSda    = OLED_SDA_PIN;
  uint8_t  oledScl    = OLED_SCL_PIN;
#endif
  uint8_t  oledAddress = OLED_ADDR;
  uint8_t  buttonPins[4] = { BTN_UP_PIN, BTN_DOWN_PIN, BTN_LEFT_PIN, BTN_RIGHT_PIN };
  // Scene courante
  uint8_t  pattern    = TP_RAINBOW;
  uint32_t primaryColor   = 0x19C37D; // couleur PogLight
  uint32_t secondaryColor = 0x6C3BFF;
  uint8_t  speed      = 50;           // 0-100 (vitesse animation)
  uint8_t  purpose    = LP_AMBIENT;   // role principal transmis a PogHome
  uint8_t  sectionCount = 0;
  uint16_t nextSectionId = 1;
  LedSection sections[MAX_SECTIONS];
  // Reseau (optionnel : web/OTA, le controleur continue sans)
  String   wifiSsid;
  String   wifiPass;
};

extern Config            g_config;
extern SemaphoreHandle_t g_configMutex;

void   configBegin();
bool   configLoad();
void   configSave();
void   configFillJson(JsonObject root, bool includeWifiPass);
void   configApplyJson(JsonObjectConst doc);
const char* lightPurposeKey(uint8_t purpose);
const char* lightPurposeLabel(uint8_t purpose);
