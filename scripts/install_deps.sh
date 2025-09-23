#!/usr/bin/env bash
set -euo pipefail

if ! command -v arduino-cli >/dev/null 2>&1; then
  echo "arduino-cli non trouvé. Sur macOS: brew install arduino-cli; sur Linux: voir https://arduino.github.io/arduino-cli" >&2
  exit 1
fi

arduino-cli config init >/dev/null 2>&1 || true
arduino-cli core update-index
arduino-cli core install esp32:esp32

arduino-cli lib update-index
arduino-cli lib install "ESP Async WebServer" || true
arduino-cli lib install "AsyncTCP" || true

echo "OK: core esp32 et bibliothèques installés."
echo "Ouvrez l’IDE Arduino et téléversez Fichier > Exemples > ESP32Server > esp32server_basic"
