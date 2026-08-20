// Test hôte de la garde de câblage — src/config.cpp, hardwarePinsValid.
//
// Pourquoi celui-là et pas un autre : c'est la seule des sept corrections dont
// la logique tienne hors du fer. Les six autres vivent dans une tâche FreeRTOS,
// dans un serveur HTTP ou dans le rendu FastLED, et se vérifient par lecture
// croisée puis à la compilation. Celle-ci décide si le ruban du salon reste
// allumé, alors elle mérite un test qui tourne sans lampe.
//
//   ./test/host/run.sh
//
#include <cstdio>

#include "config.h"

static int failures = 0;

static void check(bool condition, const char *what) {
  if (condition) {
    printf("  ok   %s\n", what);
  } else {
    printf("  ÉCHEC %s\n", what);
    ++failures;
  }
}

// La règle exacte posée dans web.cpp (handleSaveConfig, handleSetup) et dans
// pogdev.cpp (handleCommand) : on refuse la modification qui INTRODUIT la
// collision, jamais une configuration déjà en collision — sans quoi on
// verrouille l'endroit où on la répare.
static bool changeRefused(const Config &before, const Config &after) {
  return !hardwarePinsValid(after) && hardwarePinsValid(before);
}

// Le foyer : ledPin et buttonPins[3] valent tous deux GPIO 4.
static Config foyer() {
  Config c;
  c.ledPin = 4;
  c.numLeds = 60;
  c.oledEnabled = false;
  c.buttonsEnabled = false;
  c.buttonMode = BIM_DIGITAL_PULLUP;
  c.buttonPins[0] = 5;
  c.buttonPins[1] = 6;
  c.buttonPins[2] = 7;
  c.buttonPins[3] = 4;
  c.oledSda = 5;
  c.oledScl = 6;
  return c;
}

int main() {
  printf("hardwarePinsValid (SOC_TOUCH_SENSOR_SUPPORTED=%d)\n",
         (int)SOC_TOUCH_SENSOR_SUPPORTED);

  // --- le scénario qui éteint le salon -------------------------------------
  Config before = foyer();
  check(hardwarePinsValid(before),
        "boutons éteints : GPIO 4 n'est réclamé que par le ruban");

  Config after = before;
  after.buttonsEnabled = true;
  check(!hardwarePinsValid(after),
        "boutons allumés : GPIO 4 est réclamé par le ruban ET par le bouton 3");
  check(changeRefused(before, after),
        "cocher « Boutons » au portail est refusé");

  // --- table de décision (défaut 4) ----------------------------------------
  Config sain = foyer();
  Config casse = after;

  check(!changeRefused(sain, sain), "sain -> sain : accepté");
  check(changeRefused(sain, casse), "sain -> cassé : refusé");
  check(!changeRefused(casse, sain),
        "cassé -> sain : accepté, c'est la réparation");
  Config casseAutrement = casse;
  casseAutrement.numLeds = 120;
  check(!changeRefused(casse, casseAutrement),
        "cassé -> cassé : accepté, la lampe reste pilotable et réparable");

  // Une commande MQTT sans rapport ne doit jamais tomber sur la garde.
  Config allume = casse;
  allume.pattern = TP_RAINBOW;
  check(!changeRefused(casse, allume),
        "allumer la lampe sur un câblage déjà en collision : accepté");

  // --- les autres collisions -----------------------------------------------
  Config libre = foyer();
  libre.buttonPins[3] = 8;
  libre.buttonsEnabled = true;
  check(hardwarePinsValid(libre), "quatre boutons sur quatre broches libres");

  Config deuxBoutons = libre;
  deuxBoutons.buttonPins[1] = deuxBoutons.buttonPins[0];
  check(!hardwarePinsValid(deuxBoutons), "deux boutons sur la même broche");

  Config oled = foyer();
  oled.oledEnabled = true;
  oled.oledSda = 9;
  oled.oledScl = 9;
  check(!hardwarePinsValid(oled), "SDA et SCL sur la même broche");

  Config oledSurRuban = foyer();
  oledSurRuban.oledEnabled = true;
  oledSurRuban.oledSda = 4;
  oledSurRuban.oledScl = 9;
  check(!hardwarePinsValid(oledSurRuban), "l'écran réclame la broche du ruban");

  // Un périphérique éteint ne réclame rien : c'est ce qui rend le foyer valide
  // aujourd'hui malgré la collision inscrite dans sa configuration.
  Config eteints = foyer();
  eteints.oledEnabled = false;
  eteints.oledSda = 4;
  eteints.oledScl = 4;
  check(hardwarePinsValid(eteints),
        "écran éteint : ses broches ne comptent pas");

  // --- mode tactile --------------------------------------------------------
  Config tactile = libre;
  tactile.buttonMode = BIM_CAPACITIVE;
#if SOC_TOUCH_SENSOR_SUPPORTED
  check(hardwarePinsValid(tactile), "tactile accepté sur une puce qui en a");
#else
  check(!hardwarePinsValid(tactile), "tactile refusé sur une puce qui n'en a pas");
#endif

  printf(failures ? "\n%d échec(s)\n" : "\nTout passe\n", failures);
  return failures ? 1 : 0;
}
