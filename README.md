# PogLight

PogLight est un controleur autonome de bandes LED pour ESP32. Il fonctionne sans
cloud et se pilote depuis une interface web locale. Une variante ESP32-S3 prend
aussi en charge un ecran OLED et quatre boutons tactiles ; la variante ESP32-C3
SuperMini est compacte et entierement pilotee par le web.

## Fonctionnalites

- bandes adressables WS2812B et compatibles 800 kHz, jusqu'a 300 LED ;
- sortie mono analogique/PWM ;
- couleurs principale et secondaire personnalisables ;
- luminosite, vitesse, ordre RGB et sens de bande configurables ;
- limite de courant logicielle reglable pour proteger l'alimentation ;
- apercu de la bande dans le navigateur ;
- portail de configuration Wi-Fi et nom local `poglight.local` ;
- mise a jour du firmware par OTA ;
- configuration persistante en NVS ;
- fonctionnement LED independant de la connexion reseau.

### Effets

- couleur pleine ;
- arc-en-ciel ;
- chenillard ;
- respiration ;
- feu ;
- scintillement ;
- degrade deux couleurs ;
- balayage deux couleurs ;
- blanc plein et extinction.

PogLight conserve aussi trois outils de diagnostic pratiques : verification de
l'ordre des couleurs, pixel mobile pour compter les LED et remplissage progressif
pour reperer une coupure.

## Materiel

### ESP32-C3 SuperMini

La cible `esp32c3` est la version recommandee pour un controleur compact.

| Signal | Valeur par defaut |
| --- | --- |
| Data LED | GPIO 2 |
| Interface | Web uniquement |
| Flash | 4 Mo, partition OTA `min_spiffs` |

Les GPIO selectionnables depuis l'interface sont 2, 3, 4, 5, 6, 7 et 10.
Le modele exact de carte doit etre verifie avant le cablage definitif, car certaines
variantes SuperMini utilisent une LED embarquee ou des broches de boot differentes.

### ESP32-S3

| Signal | GPIO |
| --- | --- |
| Data LED par defaut | 18 |
| Sortie alternative | 16 |
| Boutons tactiles | 5, 6, 7, 4 |
| OLED SDA/SCL | 13/11, detection de l'ordre inverse |
| OLED | SSD1306/SSD1315, 128x64, adresse detectee |

### Alimentation de la bande

Utiliser une alimentation 5 V dimensionnee pour la bande, avec une masse commune
entre l'ESP32 et l'alimentation. Une resistance de 220 a 470 ohms en serie sur la
data et un condensateur d'environ 1000 uF entre 5 V et GND sont recommandes.

La limite de courant de PogLight est une protection logicielle utile, mais elle ne
remplace ni une alimentation correcte ni un fusible adapte. Une bande RGB peut
consommer jusqu'a environ 60 mA par LED en blanc a pleine puissance.

## Compiler

Le projet utilise PlatformIO :

```bash
# ESP32-C3 SuperMini
pio run -e esp32c3

# ESP32-S3 DevKit
pio run -e esp32s3

# ESP32 classique (controle web headless)
pio run -e esp32dev
```

Pour flasher une carte branchee :

```bash
pio run -e esp32c3 -t upload
pio device monitor -b 115200
```

## Premiere configuration

1. Au premier demarrage, PogLight cree le point d'acces `PogLight-Setup`.
2. Se connecter a ce reseau puis ouvrir `http://192.168.4.1`.
3. Configurer la bande et, si souhaite, le reseau Wi-Fi local.
4. Apres redemarrage, ouvrir `http://poglight.local` ou l'adresse IP affichee.

Le controleur continue d'animer les LED si le Wi-Fi est indisponible.

## Architecture

| Fichier | Role |
| --- | --- |
| `src/main.cpp` | Demarrage non bloquant, Wi-Fi, rendu a environ 60 FPS |
| `src/config.*` | Modele de configuration et persistance NVS |
| `src/leds.*` | Rendu FastLED, effets, ordre des couleurs et limite de courant |
| `src/web.*` | API HTTP, portail captif et OTA |
| `src/web_ui.h` | Interface web embarquee |
| `src/display.*` | Interface OLED et navigation locale sur ESP32-S3 |
| `src/buttons.*` | Lecture des boutons tactiles sur ESP32-S3 |

## API locale

| Methode | Route | Usage |
| --- | --- | --- |
| `GET` | `/api/state` | Etat du controleur et configuration |
| `GET` | `/api/leds` | Apercu hexadecimal des 64 premieres LED |
| `GET` | `/api/scan` | Recherche des reseaux Wi-Fi |
| `POST` | `/api/config` | Applique et persiste la configuration |
| `POST` | `/api/wifi` | Enregistre le Wi-Fi puis redemarre |
| `POST` | `/api/ota` | Installe un `firmware.bin` |
| `POST` | `/api/reboot` | Redemarre le controleur |

L'API est destinee au reseau local et ne fournit actuellement aucune
authentification. Ne pas exposer PogLight directement sur Internet.

## Releases et store POG

Le workflow GitHub Actions suit le meme contrat de release que `pog-os-airplay`.
Une pull request compile les trois cibles sans publier. Chaque push de code sur
`main`, ou lancement manuel du workflow, compile puis publie une release GitHub.

Chaque release contient :

- `firmware-<carte>.bin`, image applicative pour une mise a jour OTA ;
- `merged-<carte>.bin`, image complete pour le premier flash USB ;
- `manifest.json`, catalogue des cartes avec puce, taille et SHA-256 ;
- `SHA256SUMS`, empreintes de tous les binaires et du manifeste.

La version de depart se trouve dans `version.txt`. Si cette version existe deja,
la CI incremente automatiquement le correctif, met a jour `version.txt`, cree le
tag `vX.Y.Z` puis marque la nouvelle release comme derniere version. Le manifeste
est valide avec ses fichiers et leurs empreintes avant toute publication.
