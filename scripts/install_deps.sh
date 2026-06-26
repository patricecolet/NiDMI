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
arduino-cli lib install "AppleMIDI" || true
arduino-cli lib install "OSC" || true

# nidmi-core : clone direct dans le dossier des bibliothèques Arduino
PLATFORM="linux"
if [[ "$OSTYPE" == darwin* ]]; then PLATFORM="mac"; fi
case "$PLATFORM" in
  mac)  ARDUINO_LIBS="$HOME/Documents/Arduino/libraries" ;;
  *)    ARDUINO_LIBS="$HOME/Arduino/libraries" ;;
esac
NIDMI_CORE_DEST="$ARDUINO_LIBS/nidmi-core"
NIDMI_CORE_REMOTE="https://github.com/patricecolet/nidmi-core.git"

if [ -d "$NIDMI_CORE_DEST/.git" ]; then
  echo "nidmi-core: mise à jour (git pull)..."
  git -C "$NIDMI_CORE_DEST" pull --ff-only --quiet || echo "⚠️  git pull échoué (vérifier l'état du repo)"
elif [ ! -d "$NIDMI_CORE_DEST" ]; then
  echo "nidmi-core: clonage dans $NIDMI_CORE_DEST..."
  git clone "$NIDMI_CORE_REMOTE" "$NIDMI_CORE_DEST"
else
  echo "nidmi-core: dossier existant sans git, remplacement par un clone..."
  rm -rf "$NIDMI_CORE_DEST"
  git clone "$NIDMI_CORE_REMOTE" "$NIDMI_CORE_DEST"
fi

echo "OK: core esp32 et bibliothèques installés (dont nidmi-core)."
echo "Ouvrez l'IDE Arduino et téléversez Fichier > Exemples > NiDMI > nidmi_basic"
