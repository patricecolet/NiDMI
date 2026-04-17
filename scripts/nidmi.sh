#!/bin/bash

# Script unifié pour nidmi
# Usage: ./scripts/nidmi.sh [OPTIONS]
# 
# Options:
#   sync     - Synchroniser les fichiers seulement
#   compile  - Synchroniser + compiler
#   upload   - Synchroniser + compiler + uploader
#   flash    - Uploader seulement (sans sync ni compile ; dernier binaire arduino-cli)
#   all      - Tout faire (sync + compile + upload + test)
#   clean    - Nettoyer le cache seulement
#   help     - Afficher cette aide

set -e  # Arrêter en cas d'erreur

# Variables
<<<<<<< HEAD
REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
=======
REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}" )/.." && pwd)"
>>>>>>> 86c724ede13eadc73ef7732ba0df919866b41f8e

# Détection de la plateforme (macOS, WSL, Linux)
PLATFORM="linux"
if grep -qi microsoft /proc/version 2>/dev/null; then
    PLATFORM="wsl"
elif [[ "$OSTYPE" == darwin* ]]; then
    PLATFORM="mac"
fi

# Tentative de détection automatique du sketchbook Arduino de l'IDE
detect_sketchbook_path() {
    local prefs_file
    case "$PLATFORM" in
        mac)
            prefs_file="$HOME/Library/Arduino15/preferences.txt"
            ;;
        wsl)
            prefs_file="$HOME/.arduino15/preferences.txt"
            ;;
        linux)
            prefs_file="$HOME/.arduino15/preferences.txt"
            ;;
        *)
            return 1
            ;;
    esac

    if [ -f "$prefs_file" ]; then
        local path
        path="$(grep -E '^sketchbook.path=' "$prefs_file" 2>/dev/null | sed 's/^sketchbook.path=//')"
        if [ -n "$path" ]; then
            printf "%s" "$path"
            return 0
        fi
    fi
    return 1
}

<<<<<<< HEAD
# Dossiers Arduino (valeurs par défaut, surchargeables via ARDUINO_LIB_DIR / ARDUINO_CACHE_DIR)
=======
>>>>>>> 86c724ede13eadc73ef7732ba0df919866b41f8e
MAC_LIB_DEFAULT="$HOME/Documents/Arduino/libraries/NiDMI"
MAC_CACHE_DEFAULT="$HOME/Library/Caches/arduino/sketches"
WSL_LIB_DEFAULT="$HOME/Arduino/libraries/NiDMI"
WSL_CACHE_DEFAULT="$HOME/.arduino15/sketches"
LINUX_LIB_DEFAULT="$HOME/Arduino/libraries/NiDMI"
LINUX_CACHE_DEFAULT="$HOME/.arduino15/sketches"

<<<<<<< HEAD
# Si possible, aligner la synchro sur le sketchbook utilisé par l'IDE Arduino
=======
>>>>>>> 86c724ede13eadc73ef7732ba0df919866b41f8e
SKETCHBOOK_FROM_PREFS="$(detect_sketchbook_path || true)"

case "$PLATFORM" in
    mac)
<<<<<<< HEAD
        if [ -n "$SKETCHBOOK_FROM_PREFS" ] && [ -z "$ARDUINO_LIB_DIR" ]; then
=======
        if [ -n "$SKETCHBOOK_FROM_PREFS" ] && [ -z "${ARDUINO_LIB_DIR:-}" ]; then
>>>>>>> 86c724ede13eadc73ef7732ba0df919866b41f8e
            ARDUINO_LIB_DIR="$SKETCHBOOK_FROM_PREFS/libraries/NiDMI"
        fi
        ARDUINO_LIB_DIR="${ARDUINO_LIB_DIR:-$MAC_LIB_DEFAULT}"
        ARDUINO_CACHE_DIR="${ARDUINO_CACHE_DIR:-$MAC_CACHE_DEFAULT}"
        ;;
    wsl)
<<<<<<< HEAD
        if [ -n "$SKETCHBOOK_FROM_PREFS" ] && [ -z "$ARDUINO_LIB_DIR" ]; then
=======
        if [ -n "$SKETCHBOOK_FROM_PREFS" ] && [ -z "${ARDUINO_LIB_DIR:-}" ]; then
>>>>>>> 86c724ede13eadc73ef7732ba0df919866b41f8e
            ARDUINO_LIB_DIR="$SKETCHBOOK_FROM_PREFS/libraries/NiDMI"
        fi
        ARDUINO_LIB_DIR="${ARDUINO_LIB_DIR:-$WSL_LIB_DEFAULT}"
        ARDUINO_CACHE_DIR="${ARDUINO_CACHE_DIR:-$WSL_CACHE_DEFAULT}"
        ;;
    linux)
<<<<<<< HEAD
        if [ -n "$SKETCHBOOK_FROM_PREFS" ] && [ -z "$ARDUINO_LIB_DIR" ]; then
=======
        if [ -n "$SKETCHBOOK_FROM_PREFS" ] && [ -z "${ARDUINO_LIB_DIR:-}" ]; then
>>>>>>> 86c724ede13eadc73ef7732ba0df919866b41f8e
            ARDUINO_LIB_DIR="$SKETCHBOOK_FROM_PREFS/libraries/NiDMI"
        fi
        ARDUINO_LIB_DIR="${ARDUINO_LIB_DIR:-$LINUX_LIB_DEFAULT}"
        ARDUINO_CACHE_DIR="${ARDUINO_CACHE_DIR:-$LINUX_CACHE_DEFAULT}"
        ;;
    *)
        echo "⚠️  Plateforme non reconnue: $PLATFORM"
        exit 1
        ;;
esac

<<<<<<< HEAD
=======

>>>>>>> 86c724ede13eadc73ef7732ba0df919866b41f8e
BOARD_TYPE="s3"  # Par défaut: S3
BOARD="esp32:esp32:XIAO_ESP32S3"
DEFAULT_SKETCH="nidmi_basic"
NVS_RESET_SKETCH="nidmi_clear_nvs"
CLEAR_NVS=false
PORT_OVERRIDE=""

# Port série (optionnel, peut être forcé via --port)
SERIAL_PORT=""

# Langue par défaut (français)
LANG_CODE="fr"

# Options d'optimisation JSON
LIGHT_MODE=false
# Pagination par défaut : NiDMI nécessite la pagination pour éviter la troncature du JSON des définitions
PAGINATION_MODE=true

# Partition C3 sans SPIFFS (app ~4 Mo). Par défaut activée pour C3 uniquement (évite 97% flash).
LARGE_APP=false
NO_LARGE_APP=false
# Partition split-fs (2x LittleFS dédiés: seqfs + mapfs)
SPLIT_FS=false

# Parser les arguments pour --lang, --board, --light, --pagination, --no-pagination, --large-app, --no-large-app, --split-fs, --port
ARGS=()
while [[ $# -gt 0 ]]; do
    case $1 in
        --lang)
            LANG_CODE="$2"
            shift 2
            ;;
        --board)
            BOARD_TYPE="$2"
            shift 2
            ;;
        --port)
            PORT_OVERRIDE="$2"
<<<<<<< HEAD
=======
            SERIAL_PORT="$2"
>>>>>>> 86c724ede13eadc73ef7732ba0df919866b41f8e
            shift 2
            ;;
        --clear-nvs)
            CLEAR_NVS=true
            shift
            ;;
        --light)
            LIGHT_MODE=true
            shift
            ;;
        --pagination)
            PAGINATION_MODE=true
            shift
            ;;
        --no-pagination)
            PAGINATION_MODE=false
            shift
            ;;
        --large-app)
            LARGE_APP=true
            shift
            ;;
        --no-large-app)
            NO_LARGE_APP=true
            shift
            ;;
        --split-fs)
            SPLIT_FS=true
            shift
            ;;
        *)
            ARGS+=("$1")
            shift
            ;;
    esac
done

# Réinsérer les arguments sans --lang et --board
set -- "${ARGS[@]}"

# Définir le nom du sketch APRÈS le parsing des arguments
SKETCH_NAME="${2:-$DEFAULT_SKETCH}"  # Utiliser le 2ème argument ou le défaut
SKETCH_PATH="$ARDUINO_LIB_DIR/examples/$SKETCH_NAME/$SKETCH_NAME.ino"

# Fonction pour forcer un sketch
set_sketch() {
    SKETCH_NAME="$1"
    SKETCH_PATH="$ARDUINO_LIB_DIR/examples/$SKETCH_NAME/$SKETCH_NAME.ino"
}

# Validation de la langue
if [ "$LANG_CODE" != "fr" ] && [ "$LANG_CODE" != "en" ]; then
    echo "⚠️  Langue non supportée: $LANG_CODE, utilisation de 'fr' par défaut"
    LANG_CODE="fr"
fi

# Validation et configuration du board
case "$BOARD_TYPE" in
    c3|C3)
        BOARD="esp32:esp32:XIAO_ESP32C3"
        BOARD_TYPE="c3"
        ;;
    s3|S3)
        BOARD="esp32:esp32:XIAO_ESP32S3"
        BOARD_TYPE="s3"
        ;;
    *)
        echo "⚠️  Board non supporté: $BOARD_TYPE, utilisation de 's3' par défaut"
        BOARD="esp32:esp32:XIAO_ESP32S3"
        BOARD_TYPE="s3"
        ;;
esac

# C3 : activer --large-app par défaut (partition ~4 Mo) sauf si --no-large-app
if [[ "$BOARD" == *"XIAO_ESP32C3"* ]] && [ "$NO_LARGE_APP" != true ]; then
    LARGE_APP=true
fi

# Export pour build_html_simple.sh
export LANG_CODE

# Fonction d'aide
show_help() {
    echo "🚀 NiDMI - Script unifié"
    echo "=============================="
    echo ""
    echo "Usage: ./scripts/nidmi.sh [OPTION] [SKETCH] [--lang LANG] [--board BOARD] [--port DEVICE]"
    echo ""
    echo "Options:"
    echo "  sync     - Synchroniser les fichiers seulement"
    echo "  compile  - Synchroniser + compiler"
    echo "  build    - Synchroniser + compiler + stocker le binaire"
    echo "  upload   - Synchroniser + compiler + uploader"
    echo "  flash    - Uploader sans recompiler (binaire déjà compilé pour ce sketch + board)"
    echo "  monitor  - Ouvrir le moniteur série"
    echo "  all      - Tout faire (sync + compile + upload + test)"
    echo "  clean    - Nettoyer le cache seulement"
    echo "  help     - Afficher cette aide"
    echo ""
    echo "Options:"
    echo "  --lang LANG     - Langue de l'interface web (fr|en, défaut: fr)"
    echo "                    Nécessite jq installé (brew install jq sur macOS)"
    echo "  --large-app     - [C3] Forcer la partition sans SPIFFS (app ~4 Mo)"
    echo "  --no-large-app  - [C3] Désactiver la partition agrandie (défaut: activée pour C3)"
    echo "  --split-fs      - [C3/S3] 2 partitions LittleFS dédiées (seqfs + mapfs)"
    echo "  --board BOARD - Type de carte ESP32 (c3|s3, défaut: s3)"
    echo "                  c3 = XIAO ESP32-C3"
    echo "                  s3 = XIAO ESP32-S3"
    echo "  --port DEVICE - Port série explicite (ex: /dev/ttyUSB0, /dev/ttyACM0, /dev/cu.usbmodem*)"
    echo "  --clear-nvs   - Utiliser le sketch de reset NVS"
    echo "  --light          - Mode LIGHT: définitions simplifiées (réduit la taille JSON)"
    echo "  --pagination     - Activer la pagination (défaut: activée)"
    echo "  --no-pagination  - Désactiver la pagination (à utiliser seulement avec --large-app sur C3)"
    echo "  --port PORT      - Forcer le port série (ex: /dev/cu.usbmodem101)"
    echo ""
    echo "Variables d'environnement (optionnel) :"
    echo "  ARDUINO_LIB_DIR   Bibliothèque NiDMI (défaut: \$HOME/Documents/Arduino/libraries/NiDMI)"
    echo "  ARDUINO_CACHE_DIR Cache sketches Arduino (défaut: macOS ~/Library/Caches/... ; Linux ~/.cache/...)"
    echo ""
    echo "  Pagination : activée par défaut (C3 et S3). Évite la troncature du JSON des définitions."
    echo "  Partition C3 : --large-app est activé par défaut pour C3 (partition ~4 Mo). Utiliser --no-large-app pour désactiver."
    echo "  Split FS : optionnel via --split-fs (remplace la partition standard par app0 + seqfs + mapfs)."
    echo ""
    echo "Sketches disponibles:"
    echo "  nidmi_basic (défaut)"
    echo "  nidmi_clear_nvs (reset NVS)"
    echo ""
    echo "Exemples:"
    echo "  ./scripts/nidmi.sh sync                    # Synchroniser (français, S3 par défaut)"
    echo "  ./scripts/nidmi.sh sync --lang en          # Synchroniser en anglais"
    echo "  ./scripts/nidmi.sh compile --board c3      # C3 : pagination + partition 4 Mo par défaut"
    echo "  ./scripts/nidmi.sh compile --board c3 --no-large-app   # C3 sans partition agrandie"
    echo "  ./scripts/nidmi.sh compile --board c3 --split-fs       # C3 avec seqfs 128KB + mapfs 128KB"
    echo "  ./scripts/nidmi.sh compile --board s3 --split-fs       # S3 avec seqfs 512KB + mapfs 1MB"
    echo "  ./scripts/nidmi.sh compile --board s3      # Compiler pour ESP32-S3"
    echo "  ./scripts/nidmi.sh upload --board s3       # Uploader sur ESP32-S3"
<<<<<<< HEAD
=======
    echo "  ./scripts/nidmi.sh flash --board c3        # Reflash rapide (même build qu’après compile)"
>>>>>>> 86c724ede13eadc73ef7732ba0df919866b41f8e
    echo "  ./scripts/nidmi.sh upload --port /dev/ttyUSB0  # Uploader en forçant le port"
    echo "  ./scripts/nidmi.sh build                   # Build (S3 par défaut)"
    echo "  ./scripts/nidmi.sh upload nidmi_osc        # Upload sketch OSC"
    echo "  ./scripts/nidmi.sh upload --clear-nvs      # Upload sketch reset NVS"
    echo "  ./scripts/nidmi.sh monitor                 # Ouvrir le moniteur série"
    echo "  ./scripts/nidmi.sh all                     # Tout faire + test"
    echo "  ./scripts/nidmi.sh clean                   # Nettoyer le cache"
    echo ""
}

# Fonction de synchronisation
sync_files() {
    echo "🔄 Synchronisation des fichiers..."
    echo "   📁 Source: $REPO_DIR"
    echo "   📁 Destination: $ARDUINO_LIB_DIR"
    echo "   🌐 Langue: $LANG_CODE"
    
    # Vérifier si jq est installé (nécessaire pour les traductions)
    if [ "$LANG_CODE" != "fr" ] && ! command -v jq &> /dev/null; then
        echo "   ⚠️  jq non trouvé, impossible d'utiliser la langue $LANG_CODE"
        echo "   📝 Installation: brew install jq (macOS) ou sudo apt-get install jq (Linux)"
        echo "   📝 Utilisation du français par défaut"
        export LANG_CODE="fr"
    fi
    
    # Minifier l'UI avant la synchronisation
    if [ -f "$REPO_DIR/scripts/build_html_simple.sh" ]; then
        echo "   🗜️  Minification de l'UI..."
        cd "$REPO_DIR"
        ./scripts/build_html_simple.sh
        echo "   ✅ UI minifiée"
    else
        echo "   ⚠️  Script de minification non trouvé, synchronisation sans minification"
    fi
    
    # Nettoyer les anciens fichiers source (pour éviter les conflits après réorganisation)
    rm -rf $ARDUINO_LIB_DIR/src/* 2>/dev/null || true
    
    # Créer le dossier src/ et tous les sous-dossiers
    mkdir -p $ARDUINO_LIB_DIR/src
<<<<<<< HEAD
    mkdir -p $ARDUINO_LIB_DIR/src/{api,components,components/basic,components/multiplexer,components/distance,components/environment,components/motion,components/color,components/interface,components/actuator,components/display,config,hardware,managers,managers/complex,managers/complex/multiplexer,managers/complex/joystick,midi,midi/handlers,network,osc,processors,server,storage,ui,utils}
=======
    mkdir -p $ARDUINO_LIB_DIR/src/{api,components,components/basic,components/multiplexer,components/distance,components/environment,components/motion,components/color,components/interface,components/actuator,components/display,config,hardware,managers,managers/complex,managers/complex/multiplexer,managers/complex/joystick,mapping,midi,midi/handlers,network,osc,processors,server,ui,utils}
>>>>>>> 86c724ede13eadc73ef7732ba0df919866b41f8e
    
    # Copier les fichiers de la racine src/
    cp -f $REPO_DIR/src/nidmi_config.h $ARDUINO_LIB_DIR/src/ 2>/dev/null || true
    cp -f $REPO_DIR/src/nidmi_debug.h $ARDUINO_LIB_DIR/src/ 2>/dev/null || true
    cp -f $REPO_DIR/src/Globals.h $ARDUINO_LIB_DIR/src/ 2>/dev/null || true
    cp -f $REPO_DIR/src/NiDMI.h $ARDUINO_LIB_DIR/src/ 2>/dev/null || true
    cp -f $REPO_DIR/src/NiDMI.cpp $ARDUINO_LIB_DIR/src/ 2>/dev/null || true
    
    # Copier les sous-dossiers
    cp -f $REPO_DIR/src/api/*.cpp $ARDUINO_LIB_DIR/src/api/ 2>/dev/null || true
    cp -f $REPO_DIR/src/api/*.h $ARDUINO_LIB_DIR/src/api/ 2>/dev/null || true
    cp -f $REPO_DIR/src/components/*.h $ARDUINO_LIB_DIR/src/components/ 2>/dev/null || true
    cp -f $REPO_DIR/src/components/*.cpp $ARDUINO_LIB_DIR/src/components/ 2>/dev/null || true
    cp -f $REPO_DIR/src/components/basic/*.h $ARDUINO_LIB_DIR/src/components/basic/ 2>/dev/null || true
    cp -f $REPO_DIR/src/components/basic/*.cpp $ARDUINO_LIB_DIR/src/components/basic/ 2>/dev/null || true
    cp -f $REPO_DIR/src/components/multiplexer/*.h $ARDUINO_LIB_DIR/src/components/multiplexer/ 2>/dev/null || true
    cp -f $REPO_DIR/src/components/multiplexer/*.cpp $ARDUINO_LIB_DIR/src/components/multiplexer/ 2>/dev/null || true
    cp -f $REPO_DIR/src/components/distance/*.h $ARDUINO_LIB_DIR/src/components/distance/ 2>/dev/null || true
    cp -f $REPO_DIR/src/components/distance/*.cpp $ARDUINO_LIB_DIR/src/components/distance/ 2>/dev/null || true
    cp -f $REPO_DIR/src/components/environment/*.h $ARDUINO_LIB_DIR/src/components/environment/ 2>/dev/null || true
    cp -f $REPO_DIR/src/components/environment/*.cpp $ARDUINO_LIB_DIR/src/components/environment/ 2>/dev/null || true
    cp -f $REPO_DIR/src/components/motion/*.h $ARDUINO_LIB_DIR/src/components/motion/ 2>/dev/null || true
    cp -f $REPO_DIR/src/components/motion/*.cpp $ARDUINO_LIB_DIR/src/components/motion/ 2>/dev/null || true
    cp -f $REPO_DIR/src/components/color/*.h $ARDUINO_LIB_DIR/src/components/color/ 2>/dev/null || true
    cp -f $REPO_DIR/src/components/color/*.cpp $ARDUINO_LIB_DIR/src/components/color/ 2>/dev/null || true
    cp -f $REPO_DIR/src/components/interface/*.h $ARDUINO_LIB_DIR/src/components/interface/ 2>/dev/null || true
    cp -f $REPO_DIR/src/components/interface/*.cpp $ARDUINO_LIB_DIR/src/components/interface/ 2>/dev/null || true
    cp -f $REPO_DIR/src/components/actuator/*.h $ARDUINO_LIB_DIR/src/components/actuator/ 2>/dev/null || true
    cp -f $REPO_DIR/src/components/actuator/*.cpp $ARDUINO_LIB_DIR/src/components/actuator/ 2>/dev/null || true
    cp -f $REPO_DIR/src/components/display/*.h $ARDUINO_LIB_DIR/src/components/display/ 2>/dev/null || true
    cp -f $REPO_DIR/src/components/display/*.cpp $ARDUINO_LIB_DIR/src/components/display/ 2>/dev/null || true
    cp -f $REPO_DIR/src/config/*.cpp $ARDUINO_LIB_DIR/src/config/ 2>/dev/null || true
    cp -f $REPO_DIR/src/config/*.h $ARDUINO_LIB_DIR/src/config/ 2>/dev/null || true
    cp -f $REPO_DIR/src/hardware/*.cpp $ARDUINO_LIB_DIR/src/hardware/ 2>/dev/null || true
    cp -f $REPO_DIR/src/hardware/*.h $ARDUINO_LIB_DIR/src/hardware/ 2>/dev/null || true
    cp -f $REPO_DIR/src/managers/*.cpp $ARDUINO_LIB_DIR/src/managers/ 2>/dev/null || true
    cp -f $REPO_DIR/src/managers/*.h $ARDUINO_LIB_DIR/src/managers/ 2>/dev/null || true
    # Copier les sous-dossiers de managers (complex, etc.)
    if [ -d "$REPO_DIR/src/managers/complex" ]; then
        cp -f $REPO_DIR/src/managers/complex/*.cpp $ARDUINO_LIB_DIR/src/managers/complex/ 2>/dev/null || true
        cp -f $REPO_DIR/src/managers/complex/*.h $ARDUINO_LIB_DIR/src/managers/complex/ 2>/dev/null || true
        if [ -d "$REPO_DIR/src/managers/complex/multiplexer" ]; then
            cp -f $REPO_DIR/src/managers/complex/multiplexer/*.cpp $ARDUINO_LIB_DIR/src/managers/complex/multiplexer/ 2>/dev/null || true
            cp -f $REPO_DIR/src/managers/complex/multiplexer/*.h $ARDUINO_LIB_DIR/src/managers/complex/multiplexer/ 2>/dev/null || true
        fi
        if [ -d "$REPO_DIR/src/managers/complex/joystick" ]; then
            cp -f $REPO_DIR/src/managers/complex/joystick/*.cpp $ARDUINO_LIB_DIR/src/managers/complex/joystick/ 2>/dev/null || true
            cp -f $REPO_DIR/src/managers/complex/joystick/*.h $ARDUINO_LIB_DIR/src/managers/complex/joystick/ 2>/dev/null || true
        fi
    fi
    cp -f $REPO_DIR/src/midi/*.cpp $ARDUINO_LIB_DIR/src/midi/ 2>/dev/null || true
    cp -f $REPO_DIR/src/midi/*.h $ARDUINO_LIB_DIR/src/midi/ 2>/dev/null || true
    # Copier les handlers MIDI
    if [ -d "$REPO_DIR/src/midi/handlers" ]; then
        cp -f $REPO_DIR/src/midi/handlers/*.cpp $ARDUINO_LIB_DIR/src/midi/handlers/ 2>/dev/null || true
        cp -f $REPO_DIR/src/midi/handlers/*.h $ARDUINO_LIB_DIR/src/midi/handlers/ 2>/dev/null || true
    fi
    # Copier le moteur de mapping
    if [ -d "$REPO_DIR/src/mapping" ]; then
        cp -f $REPO_DIR/src/mapping/*.cpp $ARDUINO_LIB_DIR/src/mapping/ 2>/dev/null || true
        cp -f $REPO_DIR/src/mapping/*.h $ARDUINO_LIB_DIR/src/mapping/ 2>/dev/null || true
    fi
    cp -f $REPO_DIR/src/network/*.cpp $ARDUINO_LIB_DIR/src/network/ 2>/dev/null || true
    cp -f $REPO_DIR/src/network/*.h $ARDUINO_LIB_DIR/src/network/ 2>/dev/null || true
    cp -f $REPO_DIR/src/osc/*.cpp $ARDUINO_LIB_DIR/src/osc/ 2>/dev/null || true
    cp -f $REPO_DIR/src/osc/*.h $ARDUINO_LIB_DIR/src/osc/ 2>/dev/null || true
    cp -f $REPO_DIR/src/processors/*.cpp $ARDUINO_LIB_DIR/src/processors/ 2>/dev/null || true
    cp -f $REPO_DIR/src/processors/*.h $ARDUINO_LIB_DIR/src/processors/ 2>/dev/null || true
    cp -f $REPO_DIR/src/server/*.cpp $ARDUINO_LIB_DIR/src/server/ 2>/dev/null || true
    cp -f $REPO_DIR/src/server/*.h $ARDUINO_LIB_DIR/src/server/ 2>/dev/null || true
    cp -f $REPO_DIR/src/storage/*.cpp $ARDUINO_LIB_DIR/src/storage/ 2>/dev/null || true
    cp -f $REPO_DIR/src/storage/*.h $ARDUINO_LIB_DIR/src/storage/ 2>/dev/null || true
    cp -f $REPO_DIR/src/ui/*.cpp $ARDUINO_LIB_DIR/src/ui/ 2>/dev/null || true
    cp -f $REPO_DIR/src/ui/*.h $ARDUINO_LIB_DIR/src/ui/ 2>/dev/null || true
    cp -f $REPO_DIR/src/utils/*.cpp $ARDUINO_LIB_DIR/src/utils/ 2>/dev/null || true
    cp -f $REPO_DIR/src/utils/*.h $ARDUINO_LIB_DIR/src/utils/ 2>/dev/null || true
    
    # Copier les exemples
    mkdir -p $ARDUINO_LIB_DIR/examples
    cp -rf $REPO_DIR/examples/* $ARDUINO_LIB_DIR/examples/ 2>/dev/null || true
    
    echo "   ✅ Fichiers synchronisés"
}

# Fonction de nettoyage
clean_cache() {
    echo "🧹 Nettoyage du cache Arduino..."
    
    # Nettoyer le cache des sketches
    if [ -d "$ARDUINO_CACHE_DIR" ]; then
        rm -rf $ARDUINO_CACHE_DIR/*
        echo "   ✅ Cache sketches nettoyé: $ARDUINO_CACHE_DIR"
    fi
    
    # Nettoyer les bibliothèques staging (copies temporaires Arduino)
<<<<<<< HEAD
    case "$PLATFORM" in
        mac)
            ARDUINO_STAGING_DIR="$HOME/Library/Arduino15/staging/libraries"
            ;;
        wsl)
            ARDUINO_STAGING_DIR="$HOME/.arduino15/staging/libraries"
            ;;
        linux)
            ARDUINO_STAGING_DIR="$HOME/.arduino15/staging/libraries"
            ;;
        *)
            ARDUINO_STAGING_DIR="$HOME/.arduino15/staging/libraries"
            ;;
    esac
=======
    if [ -z "${ARDUINO_STAGING_DIR:-}" ]; then
        case "$PLATFORM" in
            mac)
                ARDUINO_STAGING_DIR="$HOME/Library/Arduino15/staging/libraries"
                ;;
            wsl|linux)
                ARDUINO_STAGING_DIR="$HOME/.arduino15/staging/libraries"
                ;;
            *)
                ARDUINO_STAGING_DIR="$HOME/.arduino15/staging/libraries"
                ;;
        esac
    fi
>>>>>>> 86c724ede13eadc73ef7732ba0df919866b41f8e
    if [ -d "$ARDUINO_STAGING_DIR" ]; then
        rm -rf "$ARDUINO_STAGING_DIR"/* 2>/dev/null || true
        echo "   ✅ Bibliothèques staging nettoyées: $ARDUINO_STAGING_DIR"
    fi
    
    # Nettoyer les dossiers build et bin dans la bibliothèque Arduino
    if [ -d "$ARDUINO_LIB_DIR" ]; then
        if [ -d "$ARDUINO_LIB_DIR/build" ]; then
            rm -rf "$ARDUINO_LIB_DIR/build"
            echo "   ✅ Dossier build supprimé: $ARDUINO_LIB_DIR/build"
        fi
        if [ -d "$ARDUINO_LIB_DIR/bin" ]; then
            rm -rf "$ARDUINO_LIB_DIR/bin"
            echo "   ✅ Dossier bin supprimé: $ARDUINO_LIB_DIR/bin"
        fi
    fi
    
    echo "   ✅ Cache Arduino nettoyé"
}

# Copie un CSV de partitions dans le package ESP32 Arduino (Arduino15)
install_partition_csv() {
<<<<<<< HEAD
    local PKG_BASE
    case "$PLATFORM" in
        mac)
            PKG_BASE="${HOME}/Library/Arduino15/packages/esp32/hardware/esp32"
            ;;
        wsl|linux)
            PKG_BASE="${HOME}/.arduino15/packages/esp32/hardware/esp32"
            ;;
        *)
            echo "   ⚠️  Plateforme non reconnue: $PLATFORM"
            return 1
            ;;
    esac
    
=======
>>>>>>> 86c724ede13eadc73ef7732ba0df919866b41f8e
    local SRC="$1"
    local DST_NAME="$2"
    local PKG_BASE=""
    local DST_DIR=""

    if [ -d "$HOME/Library/Arduino15/packages/esp32/hardware/esp32" ]; then
        PKG_BASE="$HOME/Library/Arduino15/packages/esp32/hardware/esp32"
    elif [ -d "$HOME/.arduino15/packages/esp32/hardware/esp32" ]; then
        PKG_BASE="$HOME/.arduino15/packages/esp32/hardware/esp32"
    fi

    if [ -n "$PKG_BASE" ]; then
        DST_DIR=$(ls -d "$PKG_BASE"/[0-9]*.[0-9]*.[0-9]*/tools/partitions 2>/dev/null | tail -1)
    fi

    if [ -z "$PKG_BASE" ] || [ -z "$DST_DIR" ]; then
        echo "   ⚠️  Package ESP32 non trouvé (Arduino15), installation partition ignorée"
        return 1
    fi

    if [ ! -f "$SRC" ]; then
        echo "   ⚠️  Fichier partition manquant: $SRC, installation ignorée"
        return 1
    fi

    cp "$SRC" "$DST_DIR/$DST_NAME"
    echo "   📦 Partition installée: $DST_DIR/$DST_NAME"
    return 0
}

# Copie la partition C3 sans SPIFFS dans le package Arduino (pour --large-app)
setup_c3_large_app_partition() {
    install_partition_csv "$REPO_DIR/tools/nidmi_c3_no_spiffs.csv" "nidmi_c3_no_spiffs.csv"
}

# Copie les partitions split-fs (2x LittleFS dédiés) pour C3/S3
setup_split_fs_partition() {
    if [[ "$BOARD" == *"XIAO_ESP32C3"* ]]; then
        install_partition_csv "$REPO_DIR/tools/nidmi_c3_dual_littlefs.csv" "nidmi_c3_dual_littlefs.csv"
    elif [[ "$BOARD" == *"XIAO_ESP32S3"* ]]; then
        install_partition_csv "$REPO_DIR/tools/nidmi_s3_dual_littlefs.csv" "nidmi_s3_dual_littlefs.csv"
    else
        echo "   ⚠️  --split-fs non supporté pour ce board: $BOARD"
        return 1
    fi
    return 0
}

# Fonction de compilation
compile_sketch() {
    echo "🔨 Compilation du sketch..."
    echo "   📁 Sketch: $SKETCH_PATH"
    echo "   📋 Board: $BOARD"
    
    if [ "$LIGHT_MODE" = true ]; then
        echo "   💡 Mode LIGHT activé (définitions simplifiées)"
    fi
    
    if [ "$PAGINATION_MODE" = true ]; then
        echo "   📄 Mode PAGINATION activé (chargement par pages)"
    fi
    
    if [ "$SPLIT_FS" = true ]; then
        echo "   💾 Mode SPLIT-FS activé (seqfs + mapfs en LittleFS dédiés)"
        setup_split_fs_partition || true
    elif [ "$LARGE_APP" = true ] && [[ "$BOARD" == *"XIAO_ESP32C3"* ]]; then
        echo "   📦 Mode LARGE-APP activé (partition C3 sans SPIFFS, app ~4 Mo)"
        setup_c3_large_app_partition || true
    fi
    
    if command -v arduino-cli &> /dev/null; then
        echo "   Utilisation d'arduino-cli..."
        
        # Construire les flags de build
        EXTRA_FLAGS_ARRAY=()
        
        if [ "$LIGHT_MODE" = true ]; then
            EXTRA_FLAGS_ARRAY+=("-DNIDMI_COMPONENT_DEFS_LIGHT")
        fi
        
        if [ "$PAGINATION_MODE" = true ]; then
            EXTRA_FLAGS_ARRAY+=("-DNIDMI_COMPONENT_DEFS_PAGINATION")
        fi
        
        
        # Build properties (flags + partition C3 si --large-app)
        BUILD_PROPS=()
        if [ ${#EXTRA_FLAGS_ARRAY[@]} -gt 0 ]; then
            BUILD_PROPS+=(--build-property "compiler.cpp.extra_flags=${EXTRA_FLAGS_ARRAY[*]}")
        fi
        if [ "$SPLIT_FS" = true ] && [[ "$BOARD" == *"XIAO_ESP32C3"* ]]; then
            BUILD_PROPS+=(--build-property "build.partitions=nidmi_c3_dual_littlefs")
            BUILD_PROPS+=(--build-property "upload.maximum_size=3801088")
        elif [ "$SPLIT_FS" = true ] && [[ "$BOARD" == *"XIAO_ESP32S3"* ]]; then
            BUILD_PROPS+=(--build-property "build.partitions=nidmi_s3_dual_littlefs")
            BUILD_PROPS+=(--build-property "upload.maximum_size=6684672")
        elif [ "$LARGE_APP" = true ] && [[ "$BOARD" == *"XIAO_ESP32C3"* ]]; then
            BUILD_PROPS+=(--build-property "build.partitions=nidmi_c3_no_spiffs")
            BUILD_PROPS+=(--build-property "upload.maximum_size=4063232")
        fi
        
        if [ ${#BUILD_PROPS[@]} -gt 0 ]; then
            arduino-cli compile --fqbn $BOARD "${BUILD_PROPS[@]}" $SKETCH_PATH
        else
            arduino-cli compile --fqbn $BOARD $SKETCH_PATH
        fi
        echo "   ✅ Compilation réussie"
    else
        echo "   ⚠️  arduino-cli non trouvé"
        echo "   📝 Veuillez compiler manuellement dans l'IDE Arduino"
        echo "   📝 Sketch: $SKETCH_PATH"
        echo "   📝 Board: $BOARD"
    fi
}

# Fonction de build (compilation + stockage binaire)
build_binary() {
    echo "🔨 Compilation et stockage du binaire..."
    echo "   📁 Sketch: $SKETCH_PATH"
    echo "   📋 Board: $BOARD"
    echo "   📦 Dossier de sortie: $REPO_DIR/bin"
    
    if [ "$LIGHT_MODE" = true ]; then
        echo "   💡 Mode LIGHT activé (définitions simplifiées)"
    fi
    
    if [ "$PAGINATION_MODE" = true ]; then
        echo "   📄 Mode PAGINATION activé (chargement par pages)"
    fi
    
    # Créer le dossier bin s'il n'existe pas
    mkdir -p "$REPO_DIR/bin"
    
    if command -v arduino-cli &> /dev/null; then
        echo "   Utilisation d'arduino-cli..."
        
        if [ "$SPLIT_FS" = true ]; then
            echo "   💾 Mode SPLIT-FS activé (seqfs + mapfs en LittleFS dédiés)"
            setup_split_fs_partition || true
        elif [ "$LARGE_APP" = true ] && [[ "$BOARD" == *"XIAO_ESP32C3"* ]]; then
            echo "   📦 Mode LARGE-APP activé (partition C3 sans SPIFFS)"
            setup_c3_large_app_partition || true
        fi
        
        # Construire les flags de build (même logique que compile_sketch)
        EXTRA_FLAGS_ARRAY=()
        
        if [ "$LIGHT_MODE" = true ]; then
            EXTRA_FLAGS_ARRAY+=("-DNIDMI_COMPONENT_DEFS_LIGHT")
        fi
        
        if [ "$PAGINATION_MODE" = true ]; then
            EXTRA_FLAGS_ARRAY+=("-DNIDMI_COMPONENT_DEFS_PAGINATION")
        fi
        
        BUILD_PROPS=()
        if [ ${#EXTRA_FLAGS_ARRAY[@]} -gt 0 ]; then
            BUILD_PROPS+=(--build-property "compiler.cpp.extra_flags=${EXTRA_FLAGS_ARRAY[*]}")
        fi
        if [ "$SPLIT_FS" = true ] && [[ "$BOARD" == *"XIAO_ESP32C3"* ]]; then
            BUILD_PROPS+=(--build-property "build.partitions=nidmi_c3_dual_littlefs")
            BUILD_PROPS+=(--build-property "upload.maximum_size=3801088")
        elif [ "$SPLIT_FS" = true ] && [[ "$BOARD" == *"XIAO_ESP32S3"* ]]; then
            BUILD_PROPS+=(--build-property "build.partitions=nidmi_s3_dual_littlefs")
            BUILD_PROPS+=(--build-property "upload.maximum_size=6684672")
        elif [ "$LARGE_APP" = true ] && [[ "$BOARD" == *"XIAO_ESP32C3"* ]]; then
            BUILD_PROPS+=(--build-property "build.partitions=nidmi_c3_no_spiffs")
            BUILD_PROPS+=(--build-property "upload.maximum_size=4063232")
        fi
        
        if [ ${#BUILD_PROPS[@]} -gt 0 ]; then
            arduino-cli compile --fqbn "$BOARD" --output-dir "$REPO_DIR/bin" "${BUILD_PROPS[@]}" "$SKETCH_PATH"
        else
            arduino-cli compile --fqbn "$BOARD" --output-dir "$REPO_DIR/bin" "$SKETCH_PATH"
        fi
        echo "   ✅ Binaire compilé et stocké dans bin/"
        echo "   📁 Fichiers créés:"
        ls -la "$REPO_DIR/bin/" 2>/dev/null || echo "   📁 Aucun fichier trouvé"
    else
        echo "   ⚠️  arduino-cli non trouvé"
        echo "   📝 Veuillez compiler manuellement dans l'IDE Arduino"
        echo "   📝 Sketch: $SKETCH_PATH"
        echo "   📝 Board: $BOARD"
    fi
}

# Fonction de moniteur série
monitor_serial() {
    echo "📺 Ouverture du moniteur série..."
    
    # Déterminer le port série (priorité: --port, NIDMI_PORT, auto-détection)
    if [ -n "$PORT_OVERRIDE" ]; then
        PORT="$PORT_OVERRIDE"
    elif [ -n "$NIDMI_PORT" ]; then
        PORT="$NIDMI_PORT"
    else
        case "$PLATFORM" in
            mac)
                PORT=$(ls /dev/cu.usbserial-* /dev/cu.usbmodem* /dev/cu.SLAB_USBtoUART* 2>/dev/null | head -1)
                ;;
            wsl|linux)
                PORT=$(ls /dev/ttyUSB* /dev/ttyACM* /dev/ttyS* 2>/dev/null | head -1)
                ;;
            *)
                PORT=""
                ;;
        esac
    fi
    if [ -z "$PORT" ]; then
        echo "   ❌ Aucun port série trouvé"
        echo "   📝 Ports disponibles (tty*/cu*):"
        ls /dev/ttyUSB* /dev/ttyACM* /dev/ttyS* /dev/cu.* 2>/dev/null | head -5 || echo "   📝 Aucun port trouvé"
        echo "   📝 Vérifiez que l'ESP32 est connecté et/ou utilisez --port ou NIDMI_PORT"
        exit 1
    fi
    
    echo "   📡 Port: $PORT"
    
    # Utiliser arduino-cli monitor si disponible
    if command -v arduino-cli &> /dev/null; then
        echo "   📺 Utilisation d'arduino-cli monitor..."
        echo "   📝 Pour quitter: Ctrl+C"
        echo "   📝 Pour envoyer des commandes: tapez directement"
        echo "   📝 Appuyez sur RESET de l'ESP32 pour voir les logs de démarrage"
        echo ""
        
        # Vérifier que le port est accessible
        if [ ! -r "$PORT" ]; then
            echo "   ❌ Port $PORT non accessible en lecture"
            echo "   📝 Essayez avec sudo ou vérifiez les permissions"
            exit 1
        fi
        
        # Configuration du moniteur avec arduino-cli et options robustes
        echo "   🔧 Lancement du moniteur série..."
        arduino-cli monitor -p "$PORT" \
            -c baudrate=115200
    else
        echo "   ⚠️  arduino-cli non trouvé"
        echo "   📝 Installez arduino-cli ou utilisez l'IDE Arduino"
        echo "   📝 Moniteur série: Outils → Moniteur série"
        exit 1
    fi
}

# Fonction d'upload
upload_sketch() {
    echo "📤 Upload vers l'ESP32..."
    
    if command -v arduino-cli &> /dev/null; then
        # Déterminer le port série (priorité: --port, NIDMI_PORT, auto-détection)
        if [ -n "$PORT_OVERRIDE" ]; then
            PORT="$PORT_OVERRIDE"
        elif [ -n "$NIDMI_PORT" ]; then
            PORT="$NIDMI_PORT"
        else
            case "$PLATFORM" in
                mac)
                    PORT=$(ls /dev/cu.usbserial-* /dev/cu.usbmodem* /dev/cu.SLAB_USBtoUART* 2>/dev/null | head -1)
                    ;;
                wsl|linux)
                    PORT=$(ls /dev/ttyUSB* /dev/ttyACM* /dev/ttyS* 2>/dev/null | head -1)
                    ;;
                *)
                    PORT=""
                    ;;
            esac
        fi
        if [ -z "$PORT" ]; then
            echo "   ❌ Aucun port série trouvé"
            echo "   📝 Ports disponibles (tty*/cu*):"
            ls /dev/ttyUSB* /dev/ttyACM* /dev/ttyS* /dev/cu.* 2>/dev/null | head -5 || echo "   📝 Aucun port trouvé"
            echo "   📝 Vérifiez que l'ESP32 est connecté et/ou utilisez --port ou NIDMI_PORT"
            exit 1
        fi
        
        echo "   📡 Port: $PORT"
        arduino-cli upload -p $PORT --fqbn $BOARD $SKETCH_PATH
        echo "   ✅ Upload réussi"
    else
        echo "   ⚠️  arduino-cli non trouvé"
        echo "   📝 Veuillez uploader manuellement dans l'IDE Arduino"
    fi
}

# Fonction principale
main() {
    if [ "$CLEAR_NVS" = true ]; then
        set_sketch "$NVS_RESET_SKETCH"
    fi
    case "${1:-help}" in
        "sync")
            echo "🚀 NiDMI - Synchronisation"
            echo "================================"
            sync_files
            clean_cache
            echo ""
            echo "✅ Synchronisation terminée !"
            echo "📝 Maintenant compile et upload dans l'IDE Arduino"
            ;;
               "compile")
                   echo "🚀 NiDMI - Synchronisation + Compilation"
                   echo "================================================"
                   sync_files
                   clean_cache
                   compile_sketch
                   echo ""
                   echo "✅ Compilation terminée !"
                   echo "📝 Maintenant upload dans l'IDE Arduino"
                   ;;
               "build")
                   echo "🚀 NiDMI - Synchronisation + Compilation + Stockage"
                   echo "========================================================"
                   sync_files
                   clean_cache
                   build_binary
                   echo ""
                   echo "✅ Build terminé !"
                   echo "📦 Binaire stocké dans bin/"
                   ;;
               "upload")
                   echo "🚀 NiDMI - Synchronisation + Compilation + Upload"
                   echo "====================================================="
                   sync_files
                   clean_cache
                   compile_sketch
                   upload_sketch
                   echo ""
                   echo "🎉 Processus terminé !"
                   echo ""
                   echo "📋 Prochaines étapes:"
                   echo "   1. Ouvrir http://192.168.4.1 dans le navigateur"
                   echo "   2. Ouvrir la console du navigateur (F12)"
                   echo "   3. Tester le clic sur SDA"
                   echo "   4. Vérifier les logs dans la console"
                   ;;
               "flash")
                   echo "⚡ NiDMI - Upload seul (sans sync ni compilation)"
                   echo "=================================================="
                   echo "   Utilise le dernier binaire arduino-cli pour ce sketch et ce FQBN."
                   echo "   Faire au moins un compile ou upload une fois avant, avec les mêmes options (--board, --split-fs, etc.)."
                   if [ ! -f "$SKETCH_PATH" ]; then
                       echo "   ❌ Sketch introuvable: $SKETCH_PATH"
                       echo "   📝 Lance d’abord: ./scripts/nidmi.sh sync"
                       exit 1
                   fi
                   upload_sketch
                   echo ""
                   echo "✅ Flash terminé (firmware inchangé depuis la dernière compilation)."
                   ;;
               "monitor")
                   echo "🚀 NiDMI - Moniteur série"
                   echo "============================="
                   monitor_serial
                   ;;
        "all")
            echo "🚀 NiDMI - TOUT FAIRE (Sync + Compile + Upload + Test)"
            echo "========================================================="
            sync_files
            clean_cache
            compile_sketch
            upload_sketch
            echo ""
            echo "🎉 Processus terminé !"
            echo ""
            echo "🧪 Test automatique:"
            echo "   Attente de 5 secondes pour le démarrage de l'ESP32..."
            sleep 5
                   echo "   🌐 Ouverture automatique du navigateur (Firefox)..."
                   open -a Firefox http://192.168.4.1 2>/dev/null || echo "   ⚠️  Ouvrez manuellement http://192.168.4.1 dans Firefox"
            echo ""
            echo "📋 Instructions de test:"
            echo "   1. Ouvrir la console du navigateur (F12 → Console)"
            echo "   2. Rafraîchir la page (F5)"
            echo "   3. Cliquer sur SDA dans l'interface"
            echo "   4. Vérifier les logs dans la console"
            echo "   5. Les pins I2C devraient se griser automatiquement"
            ;;
        "clean")
            echo "🚀 NiDMI - Nettoyage du cache"
            echo "==================================="
            clean_cache
            echo ""
            echo "✅ Cache nettoyé !"
            ;;
        "help"|*)
            show_help
            ;;
    esac
}

# Exécution
main "$@"