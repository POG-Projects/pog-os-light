# PogLight

<img src="assets/brand/icon.png" alt="POG Light icon" width="96">

PogLight is a standalone LED strip controller for ESP32 boards. It runs without
a cloud service and is controlled through a local, mobile-first web interface.
The ESP32-S3 variant can also use an OLED display and four local controls, while
the compact ESP32-C3 SuperMini variant is designed primarily for web control.

## Features

- WS2812B and compatible 800 kHz addressable strips, up to 300 LEDs
- Single-channel analog/PWM output
- Configurable primary and secondary colors
- Adjustable brightness, animation speed, RGB order, and strip direction
- Configurable software current limit
- Live LED strip preview in the browser
- Responsive glassmorphism interface with mobile onboarding
- Wi-Fi setup portal and local hostname at `poglight.local`
- Automatic discovery, adoption, and control from PogHome
- Up to eight independently controlled strip sections
- A specific purpose assigned to the lamp and to every section
- Automatic GitHub release detection and SHA-256-verified OTA updates
- Persistent configuration in ESP32 NVS
- LED rendering that continues even when the network is unavailable

There is no Twitch integration or dependency on an external control service.

### Effects

- Solid color
- Rainbow
- Chase
- Breathing
- Fire
- Twinkle
- Two-color gradient
- Two-color wipe
- Full white
- Off

PogLight also includes three diagnostic patterns: RGB order verification, a
moving pixel for counting LEDs or finding breaks, and a progressive fill test.

## Hardware

### ESP32-C3 SuperMini

The `esp32c3` target is recommended for a compact controller.

| Signal | Default |
| --- | --- |
| LED data | GPIO 2 |
| User interface | Web |
| Flash | 4 MB with dual OTA slots |
| OLED | Disabled, configurable |
| Local buttons | Disabled, configurable |

The LED GPIO choices exposed by the web interface are 2, 3, 4, 5, 6, 7, and
10. Verify the exact SuperMini board before wiring it permanently, because some
variants use an onboard LED or boot strapping pins differently.

### ESP32-S3

| Signal | Default |
| --- | --- |
| LED data | GPIO 18 |
| Alternative LED output | GPIO 16 |
| Local controls | GPIO 5, 6, 7, and 4 |
| OLED SDA/SCL | GPIO 13/11 |
| OLED | SSD1306/SSD1315, 128 × 64, address `0x3C` |

OLED and local-control GPIO assignments can be changed from the web interface.

### ESP32 DevKit

The `esp32dev` target supports the same headless web experience on a classic
4 MB ESP32 DevKit.

### Strip power

Use a 5 V supply sized for the strip and connect its ground to the ESP32 ground.
A 220–470 ohm series resistor on the data line and an approximately 1000 µF
capacitor between 5 V and ground are recommended.

PogLight's current limit is a useful software safeguard, but it does not replace
a correctly sized power supply or a fuse. An RGB strip can draw approximately
60 mA per LED when displaying full-brightness white.

## Build and flash

The project uses PlatformIO:

```bash
# ESP32-C3 SuperMini
pio run -e esp32c3

# ESP32-S3 DevKit
pio run -e esp32s3

# Classic ESP32 DevKit
pio run -e esp32dev
```

To flash a connected ESP32-C3 and open the serial monitor:

```bash
pio run -e esp32c3 -t upload
pio device monitor -b 115200
```

## First-time setup

1. On first boot, PogLight creates the `PogLight-Setup` access point.
2. Connect a phone or computer to that network.
3. Open `http://192.168.4.1` if the captive portal does not open automatically.
4. Follow the mobile onboarding to configure the strip and Wi-Fi network.
5. After the controller restarts, open `http://poglight.local` or its assigned
   IP address.

The controller continues rendering LED effects when Wi-Fi is unavailable.

## Sections and purposes

An addressable strip can be divided into up to eight independent sections from
the **Strip** (`Bande`) tab. Each section keeps a stable identifier, name,
physical LED range, enabled and power state, purpose, colors, effect, speed, and
brightness.

A purpose can be assigned to the complete lamp and to every section:

- Ambient
- Task lighting
- Night light
- Path lighting
- Television
- Status indicator
- Wake-up light
- Decorative

PogLight publishes every section as a separate light entity in PogHome and
includes its purpose and physical range in the entity metadata.

When no sections are configured, the full strip behaves as one light.

## OLED and local controls

The optional I²C OLED supports configurable SDA/SCL pins and addresses `0x3C`
or `0x3D`. These settings can be changed from the web interface without
recompiling the firmware.

Four local controls—up, down, left, and right—can also be enabled and assigned
to compatible GPIO pins. Two input modes are supported:

- A push button between the GPIO and ground, using the internal pull-up
- A capacitive electrode on ESP32 variants with touch-sensor support

The interface rejects duplicate GPIO assignments and conflicts with the LED or
OLED pins. Wiring changes are persisted and applied after a restart.

## PogHome integration

Once PogLight and PogHome are connected to the same local network, no IP address
needs to be entered:

1. PogLight discovers the `_poghome._tcp` service advertised by PogHome over
   mDNS.
2. It automatically appears in PogHome's list of POG devices available for
   adoption.
3. After approval, MQTT credentials are stored in ESP32 NVS and all commands
   remain inside the device's own MQTT namespace.

PogHome exposes the controller settings as native controls:

- Main scene: power, brightness, colors, effect, speed, and purpose
- Strip: GPIO, LED count, current limit, color order, direction, and
  addressable/PWM mode
- OLED and local controls: enable state, input mode, I²C address, and GPIO
  assignment
- Sections: create or remove, enable, power, name, range, purpose, colors,
  brightness, effect, and speed
- Network: SSID and Wi-Fi password replacement
- Diagnostics: Wi-Fi signal strength

Changes made from the web interface or local display are synchronized to
PogHome, and changes from PogHome are applied to the controller. Wiring and
Wi-Fi changes are persisted, acknowledged over MQTT, and then applied through
an automatic restart.

The Wi-Fi password can be replaced from PogHome but is never published back in
the MQTT state. The hardware identity `ESP-POGLIGHT-<MAC>` and adoption secret
are generated by the controller and remain stable across restarts. PogLight
automatically resumes mDNS discovery if the PogHome address changes.

## OTA updates

When connected to Wi-Fi, PogLight checks the latest release:

- Eight seconds after boot
- Every six hours
- Whenever the user selects **Check** (`Vérifier`) in the web interface

If the manifest contains a newer semantic version, the web interface proposes
the update automatically. Installation always requires user confirmation.

The controller downloads the application image matching its exact PlatformIO
target (`esp32c3`, `esp32s3`, or `esp32dev`) over TLS. While downloading, the
interface displays progress. PogLight compares the image SHA-256 with the hash
from `manifest.json` and activates the new OTA slot only after successful
verification. Manual `.bin` upload remains available as a recovery path.

## Architecture

| File | Responsibility |
| --- | --- |
| `src/main.cpp` | Non-blocking startup, Wi-Fi, and approximately 60 FPS rendering |
| `src/config.*` | Configuration model, validation, and NVS persistence |
| `src/leds.*` | FastLED rendering, effects, sections, color order, and current limiting |
| `src/web.*` | HTTP API, captive portal, and manual OTA upload |
| `src/web_ui.h` | Embedded responsive web interface and onboarding |
| `src/ota_update.*` | GitHub release detection, TLS download, and SHA-256 verification |
| `src/display.*` | OLED interface and local navigation |
| `src/buttons.*` | Digital or capacitive local-control input |
| `src/pogdev.*` | PogHome discovery, secure adoption, entity manifest, and MQTT bus |

## Local API

| Method | Route | Purpose |
| --- | --- | --- |
| `GET` | `/api/state` | Controller state and complete configuration |
| `GET` | `/api/leds` | Hexadecimal preview of the first 64 LEDs |
| `GET` | `/api/scan` | Nearby Wi-Fi network scan |
| `POST` | `/api/setup` | Save onboarding settings and restart |
| `POST` | `/api/config` | Apply and persist controller configuration |
| `POST` | `/api/wifi` | Save Wi-Fi credentials and restart |
| `GET` | `/api/update` | GitHub update versions, state, asset, and progress |
| `POST` | `/api/update/check` | Check the latest GitHub release immediately |
| `POST` | `/api/update/install` | Download, verify, and install the available release |
| `POST` | `/api/ota` | Install a manually uploaded application `.bin` |
| `POST` | `/api/reboot` | Restart the controller |

The local API currently has no authentication. Do not expose PogLight directly
to the Internet.

## CI, releases, and the POG Store

The GitHub Actions workflow follows the same release contract as
`pog-os-airplay`.

- Pull requests build all three targets without publishing a release.
- A code push to `main`, or a manual workflow run, builds and publishes a GitHub
  release.
- Markdown-only changes do not trigger the workflow.

Each release contains:

- `firmware-<board>.bin`: application image for OTA updates
- `merged-<board>.bin`: complete image for the initial USB flash
- `manifest.json`: board catalog with chip, flash size, asset names, merged
  image size, and SHA-256 hashes
- `SHA256SUMS`: hashes for every binary and the manifest

The starting version is stored in `version.txt`. If that release already
exists, CI increments the patch version, updates `version.txt`, creates the
`vX.Y.Z` tag, and marks the new release as latest. The manifest and every asset
hash are validated before publication so the release can be consumed by the POG
Store and by PogLight's built-in updater.
