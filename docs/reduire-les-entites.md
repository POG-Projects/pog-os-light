# Réduire les entités du PogLight — plan de reflash

*Écrit le 20 août 2026. Relevés du foyer datés du 15 août 2026. Le dépôt est à
la version 0.1.5 (commit `8201f86`) ; la lampe du foyer tourne encore en 0.1.4.
Tous les numéros de ligne de ce document décrivent **0.1.5**, l'arbre tel qu'il
est ici. Dans le binaire du foyer, les mêmes déclarations sont une trentaine de
lignes plus haut : ne pas les chercher au numéro exact si on lit le code de
0.1.4.*

---

## 1. Pourquoi ce document

Le propriétaire du foyer a posé la question ainsi : « le firmware du PogLight
renvoie beaucoup trop d'entités pour un contrôleur de LED, 33 ». Il a raison, et
la cause n'est pas dans POG Home. La fiche d'appareil de POG Home ne montre déjà
que 4 entités sur 33 — le reste est replié dans des sections profondes, ce qui
est exactement le comportement prévu. Le problème est en amont : le firmware
déclare comme *entité* tout ce qu'il sait de lui-même, y compris la manière dont
il est soudé, et POG Home n'a aucun moyen de deviner que « OLED · Adresse I²C »
n'est pas une lampe.

Le commit fautif annonce son intention dans son titre : *« Expose all PogLight
settings to PogHome »* (29 juillet), qui a fait passer le manifeste de 6 à 33
entités.

Le verdict rendu sur cette question tranche : **on descend à 11 entités
déclarées**. Ce document est la moitié firmware de ce verdict — ce qu'il faut
changer dans ce dépôt, dans quel ordre, et comment savoir que ça a marché.

## 2. La règle d'arbitrage

> **Une entité est un état du foyer qu'on observe ou qu'on commande parce que la
> maison vit. Ce qui ne change que quand un tournevis passe n'est pas une
> entité : c'est une propriété de l'appareil, et sa place est le portail web
> embarqué.**

Cette règle n'ôte rien à personne, parce que le portail web couvre déjà
l'intégralité de ce qu'on retire. `POST /api/config` accepte tous les champs
concernés — broches, ordre des couleurs, type de ruban, boutons, écran, et même
le découpage en sections (`src/config.cpp:77-170`, `configApplyJson`) — et
`POST /api/wifi` couvre l'appairage réseau **avec scan des réseaux
environnants** (`src/web.cpp:212-224` et `:248-259`), ce que MQTT ne saura jamais
faire. L'interface embarquée expose déjà des contrôles pour chacun de ces
réglages (`src/web_ui.h`). Tout est authentifié (`src/web_auth.cpp`, garde
`requireAuth()` en tête de chaque handler).

## 3. L'état des lieux

Avec **une** section déclarée, la 0.1.5 publie **34** entités et la 0.1.4 en
publie **33** — l'écart est `room_sync`, ajouté par le commit `cc70380` et
absent du binaire du foyer. La loi générale est :

| version | nombre d'entités déclarées |
|---|---|
| 0.1.4 | 25 + 8 N |
| 0.1.5 | 26 + 8 N |

où *N* est le nombre de sections. À *N* = 8 (`MAX_SECTIONS`, `src/config.h:24`)
cela donne 89 puis 90 entités **pour une seule lampe**. C'est cette loi, plus
que le chiffre du jour, qui rend la situation intenable : une entité déclarée
(`section_count`) en fabrique huit autres à chaque incrément.

Répartition telle que POG Home la publie aujourd'hui pour cet appareil :

| section | entités | montrée par défaut |
|---|---|---|
| `light` | 4 | oui |
| `configuration` | 2 | non |
| `matériel` | 15 | non |
| `réseau` | 2 | non |
| `sections` | 9 | non |
| `diagnostic` | 1 | non |

## 4. Ce que le firmware déclarera : 11

### 4.1 Les quatre de façade

Elles ne changent pas de rang, elles restent à découvert. Ce sont déjà les
quatre que POG Home montre : le modèle avait raison avant nous, c'est le
firmware qui ne l'a pas cru.

| clé | entité | déclarée à |
|---|---|---|
| `light` | Éclairage — allumer, éteindre, intensité, couleur | `src/pogdev.cpp:370-380` |
| `accent` | Couleur secondaire — visible depuis le canapé, écriture vivante | `:382` |
| `effect` | Effet — le seul réglage d'ambiance qui se dit à voix haute | `:383` |
| `speed` | Vitesse — se règle en regardant l'animation, sans redémarrage | `:384` |

`light` gagne au passage deux commandes (voir §5.5) : `sync_effect`, reprise de
l'entité `room_sync` supprimée, et `set_purpose`, reprise de l'entité `purpose`
supprimée.

### 4.2 Les sept rangées

Elles restent déclarées et restent écrivables **à la main**. Elles passent en
`category: "config"` (§5.1), le rang qui les sort des surfaces par défaut sans
les faire disparaître de l'interface.

> **Correction du 20 août 2026, après relecture du code livré.** La version
> initiale de ce paragraphe annonçait que le rang les sortait aussi de HomeKit :
> c'est faux, il n'existe aucun pont HomeKit dans `poghome` (seules la colonne
> `homekit_aid` et le drapeau `expose_to_homekit` existent, rien ne filtre par
> catégorie). Elle annonçait également que deux de ces réglages redémarrent la
> lampe : il y en a **trois**, `led_count` pose lui aussi `requiresReboot`
> (`src/pogdev.cpp`, branche `led_count`). L'argument est renforcé, pas
> affaibli.

| clé | rang | pourquoi elle reste | déclarée à |
|---|---|---|---|
| `wifi_signal` | `diagnostic` | le seul instrument du jour de la panne | `src/pogdev.cpp:426-433` |
| `oled_enabled` | `config` | éteindre l'écran la nuit est un geste d'habitant | `:407` |
| `buttons_enabled` | `config` | verrouiller les boutons (enfants, invités) est une décision de foyer | `:412` |
| `led_count` | `config` | l'erreur d'installation la plus visible — 32 annoncées, 60 soudées | `:399` |
| `power_limit` | `config` | sécurité électrique, réglable sans redémarrage | `:400-401` |
| `color_order` | `config` | on demande du rouge, le ruban donne du vert : boucle visuelle immédiate | `:402-403` |
| `direction` | `config` | même boucle visuelle ; le foyer l'a renommée « Sens », signe qu'on s'en sert | `:396-397` |

Trois de ces sept — `oled_enabled`, `buttons_enabled` et `led_count` —
**redémarrent la lampe** quand on les écrit (`handleCommand`, branches qui
posent `requiresReboot`). C'est acceptable pour un geste manuel, ça ne l'est pas
pour une automatisation ; c'est précisément ce que le rang `config` doit
empêcher — voir la réserve de §5.1 sur ce qu'il empêche *réellement*
aujourd'hui.

### 4.3 Les six fusionnées

Ces six-là ne disparaissent pas d'un besoin : elles disparaissent parce que la
même information voyage déjà, gratuitement, ailleurs.

| clé | ce qui la porte déjà |
|---|---|
| `section_<id>` | avec une seule section, `start=1 / end=32` désigne le ruban entier : c'est `light`, deux fois |
| `section_<id>_enabled` | « la section unique est active » et « la lampe est allumée » sont la même phrase, avec deux chemins d'écriture concurrents |
| `section_<id>_accent` | écrire l'accent du ruban le recopie déjà dans toutes les sections (`src/pogdev.cpp:677-682`) |
| `section_<id>_speed` | même preuve pour la vitesse (`:683-688`) — c'est le doublon que le propriétaire a repéré à l'œil nu |
| `section_<id>_purpose` | doublon d'un doublon : elle répète `purpose`, qui répète le `config` du trait meneur |
| `purpose` | la valeur voyage **déjà** dans `config.purpose` / `config.purpose_label` du trait `on_off` de `light` (`:377-378`) |

Les cinq premières vivent dans la boucle de sections et partent avec elle
(§5.2). La sixième, `purpose`, devient une commande de `light`.

### 4.4 Les seize retirées

Elles sortent du manifeste et restent réglables au portail web de la lampe.

| clé | pourquoi |
|---|---|
| `wifi_password` | un secret n'est jamais un état du foyer — et c'est le seul geste distant capable de sortir la lampe du réseau sans retour (§5.6) |
| `wifi_ssid` | le SSID part en clair dans chaque résumé maison envoyé au modèle ; le portail fait déjà l'appairage, avec scan |
| `section_count` | la seule entité qui **fabrique** des entités : l'écrire multiplie l'inventaire. Un état du foyer ne change pas la forme de l'inventaire |
| `section_<id>_start` | déjà portée par `config.start` du trait de la section (`src/pogdev.cpp:447`), et vaut 1 par construction avec une section |
| `section_<id>_end` | déjà portée par `config.end` (`:448`) ; vaut `led_count` par construction |
| `section_<id>_name` | un nom est du vocabulaire, pas un état ; POG Home a déjà `Entity.Name`, et le renommage humain survit au reflash |
| `led_pin` | un numéro de broche décrit le soudage. Ni ESPHome, ni Matter, ni HomeKit ne le modélisent en entité |
| `strip_mode` | ARGB ou analogique : propriété du matériel branché, posée une fois |
| `button_mode` | « Poussoirs · GPIO vers GND » est un schéma électrique |
| `button_pin_0` … `button_pin_3` | quatre broches, nées d'**une** ligne de code (`:416-419`) |
| `oled_sda`, `oled_scl` | broches I²C d'un écran éteint sur ce foyer |
| `oled_address` | `0x3C` : une coordonnée sur un bus dans la liste des objets d'une maison. Cet exemple règle le débat à lui seul |

**Total : 4 + 7 + 6 + 16 = 33**, plus `room_sync` qui devient une commande, soit
les 34 clés de la 0.1.5.

### 4.5 La nouvelle loi

| | avant (0.1.5) | après |
|---|---|---|
| une section ou aucune | 34 | **11** |
| *N* ≥ 2 sections | 26 + 8 N | 11 + 8 N |
| maximum (*N* = 8) | 90 | 75 |
| entités en façade | 4 | 4 + 4 N |

Le multi-section reste possible, mais il se déclenche désormais au portail web,
pas depuis une entité. Et les huit clés par section reviennent **à l'identique**
si une deuxième section est déclarée : mêmes clés, donc mêmes lignes en base,
donc mêmes noms et mêmes identifiants HomeKit qu'avant.

> **Décision propre à ce plan, non tranchée par le verdict :** dans la boucle de
> sections, `_name`, `_start`, `_end` et `_purpose` reçoivent
> `category: "config"`. C'est la traduction au cas multi-section du même
> raisonnement qui les retire au cas mono-section : la géométrie d'un découpage
> se pose une fois. Les quatre autres (`section_<id>`, `_enabled`, `_accent`,
> `_speed`) restent en façade, parce qu'à partir de deux sections ce sont de
> vrais gestes distincts.

## 5. Les modifications, ligne par ligne

Une seule version porte tout. Chaque flash est un risque sur un foyer qui vient
de repartir après quatorze jours de panne ; il n'y en aura qu'un.

### 5.1 Le rang d'exposition : `category`, pas un champ de plus

> **Cette section a été réécrite le 20 août 2026, contre le code.** La version
> initiale ajoutait un champ `entity_category` au protocole. Le code livré dit
> qu'il ne faut pas.

Le rang d'exposition existe déjà, et il tient dans `category` :

- `helloEntity` (`poghome/internal/integrations/pogdev/pogdev.go`) ne lit que
  `key`, `name`, `traits`, `category`. `core.Entity` n'a qu'un champ
  `Category`, et la colonne en base s'appelle `category`.
- `internal/core/exposure.go` teste `e.Category == "diagnostic"` pour poser
  `technical`, ce qui fait renvoyer un `ExposureSet{}` vide : ni tableau de
  bord, ni automatisation, ni API, ni IA par défaut — tout en laissant `Locked`
  à `false`, donc l'override humain reste possible. C'est très exactement
  « rangé, mais réglable à la main ».
- Le seul `entity_category` du dépôt appartient à l'intégration Home Assistant,
  qui le **replie** sur `category` (`integrations/homeassistant/integration.go`,
  `if *registry.EntityCategory == "diagnostic" || == "config"`). C'est le
  précédent maison, et il dit l'inverse de ce qu'on allait écrire : un seul axe.

Côté firmware, il n'y a donc **rien à ajouter au protocole**. `addEntity` écrit
déjà `entity["category"]` ; il suffit de lui passer `"config"` ou
`"diagnostic"`. Zéro changement de signature, zéro changement de protocole, et
le serveur d'aujourd'hui sait déjà lire le résultat.

**Réserve honnête, à ne pas cacher au propriétaire.** En l'état,
`"diagnostic"` range vraiment (`wifi_signal` l'émet déjà, donc rien à faire),
mais `"config"` ne range que *visuellement* : `appliance.go` s'en sert pour le
libellé de section et la profondeur d'affichage, et rien d'autre. Pour que les
six rangées sortent réellement de l'assistant et des automatisations, il faut
trois `|| e.Category == "config"` dans `internal/core/exposure.go`, à côté des
trois `e.Category == "diagnostic"` existants. Une ligne par site,
rétrocompatible, qui profite aussi aux entités HA déjà repliées en `"config"`.
C'est la moitié serveur, elle est hors de ce dépôt (§7, étape 1).

Émettre `"config"` avant que le serveur l'ait apprise ne casse rien : la
catégorie est passée sans validation et sert déjà de section d'affichage. Le
firmware ne gagne simplement pas encore le rang. C'est un vrai filet : l'ordre
est préférable, il n'est pas critique.

Le vocabulaire réellement émis après la réduction est `{light, config,
diagnostic}` — plus aucune entité n'est en `network`, `wifi_ssid` et
`wifi_password` partant tous les deux.

### 5.2 `publishHello` — `src/pogdev.cpp:356-479`

**Supprimer les déclarations** (14 clés) :

| lignes | clés supprimées |
|---|---|
| `394-395` | `purpose` |
| `398` | `led_pin` |
| `404-405` | `strip_mode` |
| `408-409` | `oled_sda`, `oled_scl` |
| `410-411` | `oled_address` |
| `413-414` | `button_mode` |
| `415-419` | `button_pin_0` … `button_pin_3` |
| `421` | `wifi_ssid` |
| `422` | `wifi_password` |
| `423-424` | `section_count` |

`addPinSelect` n'a plus d'appelant : le supprimer, ainsi que ses tables de
broches par cible et `parsePinOption`. Les tableaux `stripModes`,
`oledAddresses`, `buttonModes` et `buttonNames` deviennent inutilisés eux aussi.

**Poser le rang par `category`** sur les six rangées : `"config"` pour
`direction`, `led_count`, `power_limit`, `color_order`, `oled_enabled`,
`buttons_enabled`. `wifi_signal` est déjà en `"diagnostic"` et ne bouge pas.
C'est du même coup l'alignement sur le vocabulaire recommandé de `pogdev.md` —
`light`, `config`, `diagnostic` — au lieu de `matériel`, `configuration`,
`réseau`, `sections`, qui sont aujourd'hui classés en dernier et triés par
l'alphabet français. `network` n'a plus d'occupant après la réduction.

**Garder la boucle de sections derrière un test** :

```cpp
if (snapshot.sectionCount > 1) {
  for (uint8_t i = 0; i < snapshot.sectionCount; ++i) { ... }
}
```

sur `:435-468`. C'est le geste qui retire huit entités d'un coup sur ce foyer, et
qui neutralise la loi 8 N sans supprimer une seule clé.

**Remplacer `addRoomSyncEntity`** (appelée `:385`, définie `:109-143`) par un
trait `action` posé directement sur l'entité `light`, portant deux commandes
(§5.5).

### 5.3 `publishState` — `src/pogdev.cpp:481-563`

L'état doit refléter exactement le manifeste : une clé d'état sans entité
déclarée est du bruit sur le bus, et une entité déclarée sans état est une
entité qui n'affiche rien.

**Supprimer** : `:504` (`purpose`), `:506` (`led_pin`), `:510` (`strip_mode`),
`:512-516` (`oled_sda`, `oled_scl`, `oled_address`), `:518-519` (`button_mode`),
`:520-522` (la boucle `button_pin_*`), `:523` (`wifi_ssid`), `:524-526`
(`wifi_password`), `:527` (`section_count`).

**Garder derrière le même test `sectionCount > 1`** : la boucle `:530-556`.

**Restent publiées** : `light`, `accent`, `effect`, `speed`, `direction`,
`led_count`, `power_limit`, `color_order`, `oled_enabled`, `buttons_enabled`,
`wifi_signal` — onze clés, une par entité déclarée.

Le `purpose` disparaît de l'état sans rien perdre : il voyage dans le `config`
du trait `on_off` de `light`, et le handler de `set_purpose` continuera de poser
`schemaChanged = true`, donc de republier le manifeste avec la nouvelle valeur.

### 5.4 `handleCommand` — `src/pogdev.cpp:612-905`

**Supprimer les branches**, pas seulement les déclarations. Une entité retirée
du manifeste dont le handler écoute toujours reste une porte que plus rien ne
déclare, que plus rien n'affiche et que personne n'audite. C'est le pire des
deux mondes.

| lignes | branche supprimée |
|---|---|
| `717-722` | `led_pin` |
| `739-744` | `strip_mode` |
| `748-757` | `oled_sda`, `oled_scl` |
| `758-763` | `oled_address` |
| `767-773` | `button_mode` |
| `774-780` | `button_pin_*` |
| `781-787` | `wifi_ssid` |
| `788-793` | `wifi_password` |
| `794-818` | `section_count` |

**Conserver** `hardwarePinsValid` et `clampSectionsToStrip` : `led_count` reste
écrivable, donc le recadrage des sections reste nécessaire, et `oled_enabled` /
`buttons_enabled` restent écrivables par MQTT, donc la garde des broches reste
nécessaire elle aussi.

> **Correction.** La version initiale justifiait la conservation par « la garde
> des broches protège toujours le chemin du portail web ». C'est faux :
> `hardwarePinsValid` est statique dans le namespace anonyme de
> `src/pogdev.cpp` et n'a qu'un seul appelant, la fin de `handleCommand`. Le
> portail écrit par `configApplyJson` sans aucune validation de collision côté
> firmware — il valide côté navigateur, ce qui n'est pas la même chose. La
> bonne raison de garder la garde est MQTT, pas le portail.

**Supprimer aussi `parsePinOption`** : après le retrait des branches `led_pin`,
`oled_sda` / `oled_scl` et `button_pin_*`, elle n'a plus d'appelant, exactement
comme `addPinSelect`. Oubli de la version initiale de ce plan.

**Conserver la branche `section_*`** (`:819-892`) : elle sert dès qu'il y a deux
sections.

**Requalifier** `purpose` (`:695-700`) et `room_sync` (`:624-650`) en commandes
de `light` (§5.5).

### 5.5 `room_sync` et `purpose` deviennent des commandes

Le verdict tranche : `room_sync` **ne devient pas une 34ᵉ entité**. Une commande
coûte une ligne de catalogue ; l'entité coûtait 1 052 octets de manifeste et une
énumération complète des 19 effets.

Sur l'entité `light`, ajouter un quatrième trait `action` dont la configuration
porte deux commandes, sur le modèle exact de l'actuel `addRoomSyncEntity`
(`src/pogdev.cpp:109-143`, qui montre déjà comment déclarer un paramètre `enum`
et des paramètres `number` bornés) :

- **`sync_effect`** — paramètres inchangés (`effect` en `enum`, puis `speed`,
  `brightness`, `primary_hue`, `primary_saturation`, `secondary_hue`,
  `secondary_saturation` en `number`). `sensitive: false`, `reversible: true`.
- **`set_purpose`** — un paramètre `purpose` en `enum`, alimenté par
  `lightPurposeLabel(i)` pour `i` de 0 à `LP_COUNT`. `sensitive: false`,
  `reversible: true`.

Dans `handleCommand`, les deux branches se rattachent à `key == "light"`, sur le
nom de la commande. Le corps de `sync_effect` est repris tel quel de `:624-650`,
celui de `set_purpose` de `:695-700` (il pose `changed = schemaChanged = true`,
ce qui republie le manifeste avec le nouveau `purpose_label`).

### 5.6 L'AP de secours — **écarté, et pourquoi**

Le plan initial exigeait un point d'accès de secours : `src/main.cpp` ne démarre
l'AP **que si le SSID est vide**, donc une lampe à qui l'on a poussé une
mauvaise clé Wi-Fi ne revenait pas. Pas de portail, pas d'AP, écran et boutons
éteints sur ce foyer : le seul recours était un flash USB.

Sauf que la porte par laquelle cette mauvaise clé entrait, c'était
`wifi_password` — une entité écrivable à distance, sans aucune vérification.
**Cette PR la ferme.** Le risque contre lequel l'AP de secours assurait
disparaît dans le même mouvement que l'assurance ; il n'y a plus de recours à
rendre, parce qu'il n'y a plus rien qui prenne.

Et le prix était élevé. Un AP de secours déclenché sur une simple lecture de
`WiFi.status()` s'ouvre à chaque redémarrage de box, à chaque déauth, à chaque
changement de canal — un réseau **ouvert, sans mot de passe**, au SSID fixe
`PogLight-Setup`, avec DNS captif, servi par une lampe en service. Un rattrapage
qui s'allume à chaque hoquet du réseau n'est pas un rattrapage.

Reste le cas où le réseau change vraiment (le propriétaire change sa clé,
déménage). Il existait à l'identique avant cette PR et n'est pas aggravé par
elle : le comportement est celui de la 0.1.5 qui tourne aujourd'hui au foyer.
S'il faut y répondre un jour, ce sera dans sa propre PR, et à trois conditions
qu'un secours correct doit tenir : AP **protégé par clé WPA2**, déclenchement
sur une absence **stable** — un horodatage réarmé à chaque association, pas posé
une fois au démarrage — et une fenêtre de stabilité avant de refermer.

### 5.7 Le tampon MQTT — `src/pogdev.cpp:907-934`

`mqtt.setBufferSize(24576)` (`:913`) est calibré sur un pire cas à huit sections
que personne n'a. Le dimensionner sur le manifeste réellement construit rend de
l'ordre de 16 Kio sur les ~203 Kio de DRAM libre de l'ESP32-C3.

Deux précautions, dans cet ordre :

1. **Tester le retour de `setBufferSize`.** Un `realloc` raté renvoie `false`
   **sans** mettre à jour `bufferSize`, qui reste à 256 octets. La lampe se
   connecte, publie « online », puis échoue à publier le manifeste et l'état :
   en ligne et muette. Aujourd'hui ce retour est ignoré.
2. **Redimensionner à la hausse avant de publier**, jamais à la baisse en cours
   de session : `publishHello` peut grossir si l'habitant crée des sections
   depuis le portail. Le plus simple et le plus sûr est de calculer la taille
   après `serializeJson` et de rappeler `setBufferSize` si le manifeste dépasse,
   avant de publier.

### 5.8 Les retours ignorés — `src/pogdev.cpp:558-562`

`publishState` remet `stateDirty = false` sans vérifier ce que `mqtt.publish`
a renvoyé (`:561-562`). Sur échec, l'état est perdu jusqu'au prochain cycle de
30 secondes (`kStatePeriodMs`, `:24`) — au mieux. Poser
`stateDirty = !mqtt.publish(...)`, comme `publishHello` le fait déjà pour
`helloDirty` (`:478`).

### 5.9 Le backoff de `helloDirty` — `src/pogdev.cpp:478`, `:986`, `:997`

`if (helloDirty) publishHello();` (`:986`) est appelé dans une boucle qui tourne
toutes les 20 ms (`:997`). Une publication ratée reconstruit donc l'intégralité
du manifeste — 470 emplacements JSON, 5,9 Kio de texte — **cinquante fois par
seconde, indéfiniment**. Ajouter un `nextHelloAttempt` sur le modèle exact de
`nextReconnect` / `nextState` déjà présents dans la même boucle, avec un délai
qui double jusqu'à un plafond de l'ordre de trente secondes.

La sévérité de §5.7, §5.8 et §5.9 est proportionnelle au nombre d'entités. Les
corriger est ce qui rend le nombre supportable pendant que le parc migre.

### 5.10 L'accusé de commande — à noter, pas à livrer ici

`hardwarePinsValid` annule silencieusement toute configuration de broches
invalide : `g_config = before`, puis `changed = schemaChanged = requiresReboot =
false` (`src/pogdev.cpp:894-897`). Rien ne remonte l'échec — il n'existe aucun
sujet d'accusé, seul `pog/<id>/cmd` est écouté (`:927-928`).

Sur ce foyer, la collision est déjà armée : `led_pin` et `button_pin_3` valent
tous deux GPIO 4 ; `button_pin_1` et `oled_scl` valent tous deux GPIO 6. Allumer
`buttons_enabled` aujourd'hui ne casse rien — la lampe **ne fait rien, en
silence**, et le propriétaire conclut que POG Home est cassé.

Ce plan ne le corrige pas : l'accusé de commande est une capacité de transport
négociée (`command_ack_v1`, `pogdev.md:305-309`) qui déborde largement du sujet.
Mais il faut savoir que réduire le nombre d'entités *concentre* les réglages
survivants dans un panneau où l'installateur croit agir, ce qui rend ce silence
plus cher qu'avant. À traiter ensuite, pas ici.

## 6. Ce qui arrive à un foyer déjà installé

C'est la partie que ce plan ne peut pas livrer seul, et il faut le dire
franchement : **livrer le firmware sans le geste serveur qui l'accompagne, c'est
répondre au propriétaire sans que ça se voie.**

### 6.1 Ce que POG Home fait, tout seul, à la reconnexion

Le `hello` retenu est l'inventaire complet et courant. Tout ce que l'appareil
déclarait et ne déclare plus est donc *disparu*, pas hors ligne. La
réconciliation est explicite dans `poghome/internal/integrations/pogdev/pogdev.go:273-298` :

```go
gone := 0
for key, entityID := range previousKeys {
    if _, stillDeclared := keys[key]; stillDeclared {
        continue
    }
    gone++
    hub.MarkGone(entityID)
    ...
}
```

`MarkGone` (`poghome/internal/home/home.go:722-725`) appelle
`SetAvailability(entityID, core.AvailGone)`, ce qui écrit `availability = 'gone'`
dans `entity_state` et diffuse `entities` à l'interface. **Rien n'est supprimé.**
Le commentaire au-dessus dit pourquoi : « la ligne porte l'historique, la
curation et un identifiant d'accessoire HomeKit, et savoir s'il faut les perdre
est la décision de l'humain, pas une conséquence ».

Concrètement, après le reflash :

- **22 lignes** de la table `entities` passent en `gone`. Elles gardent leur
  `id`, leur `name` (dont le renommage « Sens du ruban » → « Sens »), leurs
  alias, leur pièce, leurs drapeaux `expose_to_ai` / `hidden` / `critical`, et
  leur `homekit_aid`.
- Leur abonnement aux commandes est retiré au passage (`pogdev.go:285-290`) :
  plus aucune commande ne leur parvient, y compris `wifi_password`.
- **Elles disparaissent de la fiche d'appareil** : `core.appliance.go:369-372`
  écarte du regroupement toute entité en `AvailGone` — donc la carte du PogLight
  montre bien 11 entités, sans section fantôme.
- **Mais elles restent dans la liste brute des entités.** `GET /api/v1/entities`
  ignore le booléen `useful` que `/appliances` et `/devices/{id}/entities`
  honorent (`poghome/internal/server/api/entities.go:164`). C'est la surface
  exacte d'où sort le chiffre « 33 ». Sans purge, le propriétaire y comptera
  toujours 33 lignes, dont 22 marquées « Disparu ».
- Les 11 lignes conservées ne bougent pas d'identité : `UpsertEntity` retrouve la
  ligne par son `unique_id` (`pogdev:<hw_id>:<key>`) et **restitue** le nom, les
  alias, la pièce, `expose_to_ai`, `expose_to_homekit`, `critical`, `hidden` et
  `homekit_aid` (`poghome/internal/server/store/entities.go:180-204`). Seuls
  `slug`, `traits` et `category` sont réécrits depuis le manifeste — ce qui fait
  que le renommage des catégories en `config` / `network` / `diagnostic` prend
  effet **tout seul** à la reconnexion, sans intervention.

### 6.2 Ce qu'il faut faire à la main

**Purger les 22 lignes disparues**, une par une, avec
`DELETE /api/v1/entities/{id}`. Le handler refuse la suppression tant que
l'intégration déclare encore l'entité — il exige `availability == gone` et
répond 409 sinon (`poghome/internal/server/api/entities.go:543-556`). C'est donc
un geste sûr par construction : impossible de supprimer par erreur une entité
vivante.

Ce que la purge coûte, définitivement :

- **Les réglages humains posés sur ces 22 entités sont perdus**, dont très
  probablement le renommage « Sens ». (Celui-là survit : `direction` reste
  déclarée. Ce sont les 22 autres qui sont concernées.)
- **Les identifiants HomeKit 21 à 54 sont brûlés à jamais.** Le compteur est une
  séquence persistante, pas un `MAX(aid)+1`, précisément pour qu'une suppression
  ne réattribue jamais un numéro libéré (`store/entities.go:12-16` et
  `migrations/00003_entities.sql:52-57`). Un accessoire supprimé ne rend pas son
  numéro : c'est ce qui empêche HomeKit de recoller silencieusement la pièce,
  les favoris et les automatisations d'un ancien accessoire à un appareil
  complètement différent.

Sans cette purge, la liste ne raccourcit pas. Avec elle, elle passe à 11.

## 7. L'ordre des opérations

Chaque étape est indépendamment sûre. **Correction du 20 août 2026 :** la
version initiale affirmait qu'aucune étape ne dépend d'une étape ultérieure.
C'est faux tant que deux automatisations vivantes citent `section_1`, d'où
l'étape 0 ci-dessous, qui est bloquante.

0. **Repointer les deux automatisations du salon.** « Salon Nuit » et « Salon
   Maison » appellent `set_hs` puis `set_brightness` sur `light.ambiance`
   (= `section_1`), qui disparaît avec la garde mono-section. Le moteur
   préautorise le run **entier** avant son premier effet de bord : dès que la
   cible est en `gone`, tout le run échoue, y compris le `turn_on` sur
   `light.clairage` qui le précède. La branche d'extinction, elle, ne cite que
   `light.clairage` et survit — la lampe ne s'allumerait donc plus jamais toute
   seule et continuerait de s'éteindre toute seule. Repointer les quatre appels
   vers `light.clairage` avant le flash : avec une seule section, `start=1 /
   end=32` désigne le ruban entier, le comportement est identique avant comme
   après. Attention, `UpdateAutomation` force `enabled=0` à chaque écriture :
   il faut réactiver les deux règles après modification.
1. **POG Home apprend à ranger sur `config`.** Ajouter les trois
   `|| e.Category == "config"` de §5.1 dans `internal/core/exposure.go`, à côté
   des `e.Category == "diagnostic"` existants, plus le cas de test qui va avec.
   **Ne pas ajouter de champ `entity_category`.** Déployer à une fenêtre choisie
   par le propriétaire — un déploiement de poghome coupe le hub du foyer, et
   l'effet est immédiat sur toutes les entités déjà en `"config"`, y compris
   celles que l'intégration Home Assistant y replie. Un firmware qui émettrait
   `"config"` avant ne casse rien, il ne gagne simplement pas le rang.
2. **Écrire le firmware** : §5.1 à §5.9, dans une seule branche, une seule PR,
   une seule version. Ne pas fractionner : chaque flash est un risque.
3. **Vérifier au banc, pas au foyer.** Sur une lampe de test : manifeste à 11
   entités, portail joignable, et — SSID valide mais clé fausse — **aucun** AP
   ouvert qui apparaisse, la station continuant seule ses tentatives.
4. **Publier la version.** La CI construit et publie sur `push` vers `main`
   (`.github/workflows/ci-release.yml`), en ignorant les chemins `**.md` — ce
   document seul ne déclenche donc aucun build. **Publier une release ne
   déploie rien** : l'OTA vérifie toutes les six heures (`ota_update.h`,
   `kCheckPeriodMs`) mais n'installe **que** sur demande explicite depuis
   l'interface web (`src/web.cpp:295-303`). Publier est sans effet sur le foyer.
5. **Choisir une fenêtre avec le propriétaire, puis installer** depuis le
   portail de la lampe. C'est le seul moment où le foyer bouge.
6. **Vérifier** (§8).
7. **Purger les 22 entités disparues** (§6.2), après avoir vérifié qu'elles sont
   bien en `gone` et que les 11 restantes sont bien vivantes.

**Ce qu'il ne faut surtout pas faire :** installer la 0.1.5 telle quelle. Elle
ferait passer le foyer de 33 à 34 entités et le manifeste de 5 903 à 6 955
octets. Répondre « on réduit » puis livrer une mise à jour qui augmente serait
le pire enchaînement possible pour la confiance du propriétaire. **La 0.1.5 ne
doit pas partir en l'état.**

## 8. Comment vérifier

Dans l'ordre, du plus proche du fer au plus proche de l'habitant. Tout est en
lecture seule.

**1. La lampe elle-même.** Sur le port série, `publishHello` imprime son propre
résultat (`src/pogdev.cpp:476-477`) :

```
[PogHome] manifeste: 11 entités, NNNN octets, publié
```

C'est l'instrument le plus direct : le nombre attendu est **11**, et le mot est
**publié**.

Les octets se lisent au passage. Mesurés au banc le 20 août 2026, en rejouant
`publishHello` sur l'hôte avec la même ArduinoJson que la cible, un ruban de 32
LED et une section nommée « Ambiance » :

| version | 0 section | 1 section | 2 sections | 8 sections |
|---|---|---|---|---|
| 0.1.4 | — | 33 ent. · 5 963 o | — | 89 ent. · 16 263 o |
| 0.1.5 | 26 ent. · 5 469 o | 34 ent. · 6 955 o | 42 ent. · 8 450 o | 90 ent. · 17 416 o |
| **réduite** | **11 ent. · 2 896 o** | **11 ent. · 2 896 o** | 27 ent. · 5 837 o | 75 ent. · 14 683 o |

Soit **34 → 11 entités et 6 955 → 2 896 octets** par rapport à l'arbre, **33 →
11 et 5 963 → 2 896** par rapport au binaire qui tourne au foyer. Le chiffre de
5 903 annoncé plus haut venait du port série du foyer : l'écart d'une soixantaine
d'octets tient au nom réel de la section et au `hw_id`, pas au code.

**2. Le bus.** Le `hello` est retenu, donc lisible sans rien provoquer :

```
mosquitto_sub -h <broker> -t 'pog/<device_id>/hello' -C 1 | jq '.entities | length'
mosquitto_sub -h <broker> -t 'pog/<device_id>/hello' -C 1 | jq -r '.entities[] | "\(.key)\t\(.category)"'
```

Attendu : 11 clés, quatre en `light`, six en `config`, une en `diagnostic`, et
**aucune** clé
`wifi_password`, `wifi_ssid`, `led_pin`, `button_pin_*`, `oled_sda`, `oled_scl`,
`oled_address`, `strip_mode`, `button_mode`, `section_count`, `purpose`,
`room_sync`, `section_*`.

**3. Le journal de POG Home.** La réconciliation s'annonce
(`pogdev.go:296-298`) :

```
entités disparues du descripteur  device=PogLight  nombre=22
```

Vingt-deux, pas vingt et un ni vingt-trois. Un autre chiffre veut dire qu'une
clé a changé de nom sans qu'on l'ait voulu — auquel cas une entité neuve est
apparue à côté d'une entité disparue, et la curation est perdue pour rien.

**4. La base, en lecture seule.** Toujours avec `-readonly` :

```sh
sqlite3 -readonly poghome.db "
  SELECT s.availability, COUNT(*)
  FROM entities e JOIN entity_state s ON s.entity_id = e.id
  WHERE e.unique_id LIKE 'pogdev:%'
    AND e.device_id = (SELECT id FROM devices WHERE name = 'PogLight')
  GROUP BY s.availability;"
```

Attendu avant purge : 11 vivantes (`online`, `restored` ou `unknown`) et 22
`gone`. Après purge : 11 lignes au total.

**5. L'API.** La fiche d'appareil doit montrer 11 entités réparties en trois
sections seulement (`light`, `config`, `diagnostic`), dont 4 en façade :

```
GET /api/v1/appliances
GET /api/v1/devices/{id}/entities
```

**6. Le mot de passe Wi-Fi.** C'est la vérification qui compte le plus. Dans le
manifeste : la clé `wifi_password` a disparu (point 2). Dans la base : la ligne
`text.mot_de_passe_wi_fi` est en `gone` (point 4), puis absente après purge.
Dans le firmware : la branche `key == "wifi_password"` n'existe plus, donc même
un message publié sur `pog/<id>/cmd` ne trouve plus de handler.

**7. La lumière.** Allumer, éteindre, changer la couleur, changer l'effet,
changer la vitesse — depuis POG Home, et à la voix. Puis ouvrir
`http://poglight.local` et vérifier que les vingt réglages retirés sont bien
tous là, réglables, authentifiés.

## 9. Les trois autres familles de firmware

**pog-os-sensor, pog-os-jarvis et pog-os-airplay ne sont pas concernés par la
réduction.** Vérifié dans leur code : aucun des trois ne déclare de broche, de
type de matériel, d'adresse de bus ni d'identifiant Wi-Fi en entité. Ils
déclarent entre douze et vingt entités, et chacune est un état du foyer.

- **pog-os-sensor** (`src/pogdev.cpp:272-363`) : des mesures conditionnées à la
  présence réelle du capteur (`if (environmentHasCo2())`…), la lumière de
  présence si elle est installée, le signal Wi-Fi et un état de connectivité.
  Le nombre d'entités s'adapte au matériel branché — c'est le contraire du
  PogLight, qui déclare tout ce qu'il pourrait avoir.
- **pog-os-jarvis** (`src/pogdev.cpp:298-389`) : même construction, même
  conditionnement.
- **pog-os-airplay** (`main/pogdev_app.c:110-186`) : lecture en cours,
  transport, volume, égaliseur, bande LED, veille de l'ampli, signal Wi-Fi.

Ils utilisent déjà le vocabulaire recommandé — `media`, `audio`, `light`,
`climate`, `power`, `config`, `diagnostic` — là où PogLight a inventé
`matériel`, `configuration`, `réseau` et `sections`. **PogLight est l'exception,
pas la règle.** Le désordre est local : il n'y a pas de correction de parc à
faire.

Rien ne les concerne non plus dans §5.1, puisqu'il n'y a finalement aucun champ
ajouté au protocole : le rang passe par `category`, qu'ils émettent déjà avec le
bon vocabulaire. **Correction du 20 août 2026 :** la version initiale de ce
paragraphe leur demandait d'adopter un futur champ `entity_category` dans leurs
aides de déclaration (`pog-os-sensor`, `pog-os-jarvis`, `pog-os-airplay`). Ce
champ n'existe pas et n'existera pas.

Rien ne presse : leur vocabulaire actuel garde le comportement d'aujourd'hui, et ces
trois-là n'ont rien à ranger. La bonne occasion est leur prochaine version, pas
un reflash dédié. `pog-os-airplay` en particulier se compile via la CI et non en
local — voir la note d'atelier correspondante avant d'y toucher.

## 10. Ce que ce plan ne fait pas, et pourquoi

- **Il ne bouge pas le foyer.** Aucun ordre vers la lampe, aucun flash, aucun
  redémarrage de service. Quinze des dix-neuf entités d'installation arment un
  redémarrage 1,2 s plus tard (`src/pogdev.cpp:565-569`) et deux d'entre elles
  peuvent ne pas revenir. Le foyer vient de repartir après quatorze jours de
  panne.
- **Il ne corrige pas l'exposition côté serveur.** Le rang `config`, la lecture
  du drapeau `password: true`, le filtrage par catégorie dans
  `DefaultExposeToAI` et l'action groupée d'exposition dans `pog-web` sont la
  moitié serveur du verdict. **Correction du 20 août 2026 :** la version
  initiale de ce paragraphe affirmait qu'une partie était déjà écrite et non
  commitée dans l'arbre de `poghome`. C'est faux. `git status` dans `poghome`
  ne montre que `docs/enquetes/` non suivi, HEAD à `0b9bbd7`, aucun fichier Go
  modifié. **L'étape 1 de §7 est à écrire intégralement.** Sans elle, ce plan
  réduit le nombre mais ne change pas ce que l'assistant peut faire aux entités
  restantes.
- **Il ne rend pas le câblage réglable à distance.** Vingt réglages quittent POG
  Home. Il faudra être dans le foyer, sur `http://poglight.local`. C'est
  acceptable parce qu'ils se posent une fois, le ruban dans les mains. Ce n'est
  pas acceptable si l'on installe pour quelqu'un d'autre à distance : ce cas
  existe, et il n'est pas couvert. À traiter par une vraie fonction de
  provisionnement, pas en laissant traîner trente-trois entités dans une
  maison.
- **Il ne livre pas l'accusé de commande** (§5.10), qui devient plus nécessaire
  après cette réduction qu'avant.
- **Il ne rejoue pas les mesures du verdict.** Les chiffres d'entités, de
  catégories, d'exposition et de collisions de broches ont été relevés sur le
  foyer le 15 août 2026, en lecture seule. Les numéros de ligne de ce document
  ont été relus dans l'arbre le 20 août 2026, en 0.1.5. Les tailles de manifeste
  après modification (§8) sont à mesurer au banc : personne ne les a encore
  vues.

## 11. Ce que la branche `feat/reduire-les-entites` livre réellement

Écrit le 20 août 2026, après application. Aucun appareil touché, `version.txt`
laissé à `0.1.5`, aucune release publiée : le flash reste une décision humaine
séparée.

**Le manifeste.** 34 → **11** entités et 6 955 → **2 896** octets à une section
(mesuré, §8). Les quatre de façade gardent leur clé et leur catégorie `light` ;
les six rangées passent en `config` ; `wifi_signal` reste en `diagnostic`.

**`room_sync` et `purpose` deviennent des commandes.** `addRoomSyncEntity` est
remplacée par `addLightActions`, qui pose un quatrième trait `action` sur
`light` portant `sync_effect` (paramètres inchangés) et `set_purpose`. Le trait
`action` a le domaine `switch` côté POG Home et ne déloge pas `light` dans
`DeriveKind` : le slug `light.clairage` est préservé. Dans `handleCommand`, les
deux branches sont rangées **dans** `key == "light"`, sur le nom de la commande.

**La garde de section.** `if (snapshot.sectionCount > 1)` enveloppe la boucle
dans `publishHello` **et** dans `publishState`, avec la même condition
littérale. À deux sections et plus, les huit clés reviennent à l'identique.

**Ce qui est supprimé pour de bon** : `addPinSelect` et ses tables de broches,
`parsePinOption`, les tables `stripModes` / `oledAddresses` / `buttonModes` /
`buttonNames`, et les branches `handleCommand` de `led_pin`, `strip_mode`,
`oled_sda`, `oled_scl`, `oled_address`, `button_mode`, `button_pin_*`,
`wifi_ssid`, `wifi_password` et `section_count`. Une porte que plus rien ne
déclare et que personne n'audite est le pire des deux mondes.

**Le portail n'a rien eu à gagner.** Vérifié champ par champ dans `web_ui.h` et
`configApplyJson` : les vingt réglages retirés ont tous déjà leur contrôle —
`numLeds`, `ledPin`, `colorOrder`, `reverse`, `maxMilliAmps`, `analog`,
`purpose`, `oledEnabled` / `oledSda` / `oledScl` / `oledAddress`,
`buttonsEnabled` / `buttonMode` / `buttonPin0..3`, l'éditeur de sections complet
(ajout, retrait, nom, première et dernière LED, utilité, couleurs, vitesse,
« Active »), `ssid` et `wifiPass` avec scan. Aucun réglage n'existait uniquement
comme entité MQTT.

**Le secours réseau (§5.6).** Écarté. `main.cpp` est inchangé par rapport à la
0.1.5 : aucun point d'accès ne s'ouvre tant qu'un SSID est configuré. La lampe
s'appuie sur la reconnexion automatique d'Arduino, comme aujourd'hui au foyer.

**Le tampon MQTT (§5.7).** `ensureMqttBuffer` ne redimensionne **qu'à la
hausse**, teste le retour, le trace, et refuse explicitement au-delà de 65 519
octets utiles au lieu de laisser le `size_t` se tronquer sur le `uint16_t` de
`setBufferSize`. Le tampon est dimensionné **à la connexion sur le découpage
réel** (`mqttPayloadBudget`) — 4 Kio à une section, 20 Kio au pire cas de huit,
contre 24 Kio réservés à chaque connexion auparavant. Redimensionner à la
demande en cours de session restait un pari sur l'état du tas : le realloc qui
échoue est celui d'une lampe qui tourne depuis des semaines.

**Les retours ignorés (§5.8, §5.9).** `stateDirty` reflète le retour de
`mqtt.publish`, et `helloDirty` comme `stateDirty` sont réessayés avec un délai
qui double de 500 ms à 30 s. Le second est indissociable du premier : tester le
retour de l'état sans lui aurait recréé exactement la boucle que §5.9 ferme.

**Le refus silencieux (§5.10), partiellement.** L'accusé de commande n'est
toujours pas livré, mais le revert de `hardwarePinsValid` n'est plus muet : il
trace la collision sur le port série avec l'adresse du portail, et republie
l'état pour que l'interrupteur revienne à sa position réelle au lieu de paraître
accepté. C'est important maintenant que `oled_enabled` et `buttons_enabled`
restent écrivables alors que les broches ne sont plus déclarées : sur ce foyer,
`led_pin` et `button_pin_3` valent tous deux GPIO 4, et allumer les boutons
depuis POG Home ne peut que se corriger au portail.

**Ce qui n'est pas dans cette branche** : l'étape 0 (repointer les deux
automatisations du salon) et l'étape 1 (les trois `|| e.Category == "config"`
dans `poghome/internal/core/exposure.go`) — elles vivent dans l'autre dépôt et
sur le foyer. La route `/api/rollback` et le report de
`esp_ota_mark_app_valid_cancel_rollback()` relèvent de la sûreté du chemin de
flash, pas de la réduction, et méritent leur propre version.
