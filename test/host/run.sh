#!/bin/sh
# Compile src/config.cpp sur l'hôte avec un substitut minimal d'Arduino et
# exécute le test de la garde de câblage, une fois par famille de puce : la
# règle du mode tactile dépend de SOC_TOUCH_SENSOR_SUPPORTED, et la cible C3
# du foyer ne l'a pas.
#
# Aucun matériel requis, aucun réseau touché.
set -e
root=$(cd "$(dirname "$0")/../.." && pwd)
json=$(ls -d "$root"/.pio/libdeps/*/ArduinoJson/src 2>/dev/null | head -1)
if [ -z "$json" ]; then
  echo "ArduinoJson introuvable : lancer d'abord 'pio run -e esp32c3'." >&2
  exit 1
fi
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
status=0

run() {
  name=$1; shift
  echo "=== $name ==="
  c++ -std=gnu++17 -Wall -Wextra -Wno-unused-parameter -o "$out/t" \
      -I "$root/test/host/shim" -I "$json" -I "$root/src" \
      -DARDUINOJSON_ENABLE_ARDUINO_STRING=1 \
      -DARDUINOJSON_ENABLE_ARDUINO_STREAM=0 \
      -DARDUINOJSON_ENABLE_ARDUINO_PRINT=0 \
      -DARDUINOJSON_ENABLE_PROGMEM=0 \
      "$@" "$root/test/host/test_hardware_pins.cpp" "$root/src/config.cpp"
  "$out/t" || status=1
  echo
}

# ESP32-C3 : la cible du foyer, sans capteur tactile.
run "esp32c3" -DCONFIG_IDF_TARGET_ESP32C3=1 -DDEFAULT_LED_PIN=2 \
    -DSOC_TOUCH_SENSOR_SUPPORTED=0
# ESP32-S3 : écran et boutons par défaut, tactile disponible.
run "esp32s3" -DSOC_TOUCH_SENSOR_SUPPORTED=1

exit $status
