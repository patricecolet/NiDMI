#!/bin/bash

# Script unifié pour esp32server
# Usage: ./scripts/esp32server.sh [OPTIONS]
# 
# Options:
#   sync     - Synchroniser les fichiers seulement
#   compile  - Synchroniser + compiler
#   upload   - Synchroniser + compiler + uploader
#   all      - Tout faire (sync + compile + upload + test)
#   clean    - Nettoyer le cache seulement
#   help     - Afficher cette aide

set -e  # Arrêter en cas d'erreur

# Variables
REPO_DIR="/Users/patricecolet/repo/NiDMI"
ARDUINO_LIB_DIR="/Users/patricecolet/Documents/Arduino/libraries/NiDMI"
ARDUINO_CACHE_DIR="/Users/patricecolet/Library/Caches/arduino/sketches"
BOARD="esp32:esp32:XIAO_ESP32C3"
DEFAULT_SKETCH="esp32server_basic"
SKETCH_NAME="${2:-$DEFAULT_SKETCH}"  # Utiliser le 2ème argument ou le défaut
SKETCH_PATH="$ARDUINO_LIB_DIR/examples/$SKETCH_NAME/$SKETCH_NAME.ino"

# Langue par défaut (français)
LANG_CODE="fr"

# Parser les arguments pour --lang
ARGS=()
while [[ $# -gt 0 ]]; do
    case $1 in
        --lang)
            LANG_CODE="$2"
            shift 2
            ;;
        *)
            ARGS+=("$1")
            shift
            ;;
    esac
done

# Réinsérer les arguments sans --lang
set -- "${ARGS[@]}"

# Validation de la langue
if [ "$LANG_CODE" != "fr" ] && [ "$LANG_CODE" != "en" ]; then
    echo "⚠️  Langue non supportée: $LANG_CODE, utilisation de 'fr' par défaut"
    LANG_CODE="fr"
fi

# Export pour build_html_simple.sh
export LANG_CODE

# Fonction d'aide
show_help() {
    echo "🚀 ESP32Server - Script unifié"
    echo "=============================="
    echo ""
    echo "Usage: ./scripts/esp32server.sh [OPTION] [SKETCH] [--lang LANG]"
    echo ""
    echo "Options:"
    echo "  sync     - Synchroniser les fichiers seulement"
    echo "  compile  - Synchroniser + compiler"
    echo "  build    - Synchroniser + compiler + stocker le binaire + générer UF2"
    echo "  upload   - Synchroniser + compiler + uploader"
    echo "  monitor  - Ouvrir le moniteur série"
    echo "  all      - Tout faire (sync + compile + upload + test)"
    echo "  clean    - Nettoyer le cache seulement"
    echo "  help     - Afficher cette aide"
    echo ""
    echo "Options de langue:"
    echo "  --lang LANG  - Langue de l'interface web (fr|en, défaut: fr)"
    echo "                 Nécessite jq installé (brew install jq sur macOS)"
    echo ""
    echo "Sketches disponibles:"
    echo "  esp32server_basic (défaut)"
    echo "  esp32server_osc"
    echo "  components_basic"
    echo "  rtpmidi/*"
    echo ""
    echo "Exemples:"
    echo "  ./scripts/esp32server.sh sync                # Synchroniser (français par défaut)"
    echo "  ./scripts/esp32server.sh sync --lang en      # Synchroniser en anglais"
    echo "  ./scripts/esp32server.sh upload --lang en    # Uploader avec interface anglaise"
    echo "  ./scripts/esp32server.sh compile             # Synchroniser + compiler"
    echo "  ./scripts/esp32server.sh build               # Synchroniser + compiler + stocker binaire + UF2"
    echo "  ./scripts/esp32server.sh upload esp32server_osc  # Upload sketch OSC"
    echo "  ./scripts/esp32server.sh monitor             # Ouvrir le moniteur série"
    echo "  ./scripts/esp32server.sh all                 # Tout faire + test"
    echo "  ./scripts/esp32server.sh clean               # Nettoyer le cache"
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
    
    # Copier tous les fichiers source
    cp -f $REPO_DIR/src/*.cpp $ARDUINO_LIB_DIR/src/ 2>/dev/null || true
    cp -f $REPO_DIR/src/*.h $ARDUINO_LIB_DIR/src/ 2>/dev/null || true
    cp -f $REPO_DIR/src/api/*.cpp $ARDUINO_LIB_DIR/src/api/ 2>/dev/null || true
    cp -f $REPO_DIR/src/api/*.h $ARDUINO_LIB_DIR/src/api/ 2>/dev/null || true
    cp -f $REPO_DIR/src/components/*.h $ARDUINO_LIB_DIR/src/components/ 2>/dev/null || true
    cp -f $REPO_DIR/src/midi/*.cpp $ARDUINO_LIB_DIR/src/midi/ 2>/dev/null || true
    cp -f $REPO_DIR/src/midi/*.h $ARDUINO_LIB_DIR/src/midi/ 2>/dev/null || true
    
    # Copier les exemples
    mkdir -p $ARDUINO_LIB_DIR/examples
    cp -rf $REPO_DIR/examples/* $ARDUINO_LIB_DIR/examples/ 2>/dev/null || true
    
    echo "   ✅ Fichiers synchronisés"
}

# Fonction de nettoyage
clean_cache() {
    echo "🧹 Nettoyage du cache Arduino..."
    if [ -d "$ARDUINO_CACHE_DIR" ]; then
        rm -rf $ARDUINO_CACHE_DIR/*
        echo "   ✅ Cache nettoyé"
    else
        echo "   ⚠️  Cache Arduino non trouvé"
    fi
}

# Fonction de compilation
compile_sketch() {
    echo "🔨 Compilation du sketch..."
    echo "   📁 Sketch: $SKETCH_PATH"
    echo "   📋 Board: $BOARD"
    
    if command -v arduino-cli &> /dev/null; then
        echo "   Utilisation d'arduino-cli..."
        arduino-cli compile --fqbn $BOARD $SKETCH_PATH
        echo "   ✅ Compilation réussie"
    else
        echo "   ⚠️  arduino-cli non trouvé"
        echo "   📝 Veuillez compiler manuellement dans l'IDE Arduino"
        echo "   📝 Sketch: $SKETCH_PATH"
        echo "   📝 Board: $BOARD"
    fi
}

# Fonction de génération UF2 en bash pur (sans Python)
generate_uf2() {
    echo "📦 Génération du fichier UF2..."
    
    # Déterminer le nom de base du sketch
    SKETCH_BASENAME=$(basename "$SKETCH_PATH" .ino)
    BIN_DIR="$REPO_DIR/bin"
    MERGED_BIN="$BIN_DIR/${SKETCH_BASENAME}.ino.merged.bin"
    UF2_FILE="$BIN_DIR/${SKETCH_BASENAME}.uf2"
    
    # Vérifier que le fichier merged.bin existe
    if [ ! -f "$MERGED_BIN" ]; then
        echo "   ⚠️  Fichier merged.bin non trouvé: $MERGED_BIN"
        return 1
    fi
    
    # Vérifier que xxd est disponible (nécessaire pour la conversion)
    if ! command -v xxd &> /dev/null; then
        echo "   ⚠️  xxd non trouvé, impossible de générer UF2"
        echo "   📝 xxd est généralement inclus dans les systèmes Unix"
        return 1
    fi
    
    # Déterminer l'adresse de base selon la carte ESP32
    BASE_ADDR=0
    if [[ "$BOARD" == *"XIAO_ESP32C3"* ]]; then
        BASE_ADDR=0
    elif [[ "$BOARD" == *"XIAO_ESP32S3"* ]]; then
        BASE_ADDR=0
    fi
    
    echo "   🔨 Conversion en UF2 (adresse: 0x$(printf '%08x' $BASE_ADDR))..."
    
    # Obtenir la taille du fichier binaire
    if [[ "$OSTYPE" == "darwin"* ]]; then
        BIN_SIZE=$(stat -f%z "$MERGED_BIN" 2>/dev/null)
    else
        BIN_SIZE=$(stat -c%s "$MERGED_BIN" 2>/dev/null)
    fi
    
    if [ -z "$BIN_SIZE" ] || [ "$BIN_SIZE" -eq 0 ]; then
        echo "   ❌ Impossible de déterminer la taille du fichier ou fichier vide"
        return 1
    fi
    
    # Calculer le nombre de blocs nécessaires (476 bytes de données par bloc)
    DATA_PER_BLOCK=476
    NUM_BLOCKS=$(( (BIN_SIZE + DATA_PER_BLOCK - 1) / DATA_PER_BLOCK ))
    
    # Créer le fichier UF2
    rm -f "$UF2_FILE"
    
    # Lire le fichier binaire et créer les blocs UF2
    BLOCK_NUM=0
    OFFSET=0
    
    while [ $OFFSET -lt $BIN_SIZE ]; do
        # Calculer la taille des données pour ce bloc
        REMAINING=$((BIN_SIZE - OFFSET))
        BLOCK_DATA_SIZE=$((REMAINING < DATA_PER_BLOCK ? REMAINING : DATA_PER_BLOCK))
        
        # Adresse flash pour ce bloc
        FLASH_ADDR=$((BASE_ADDR + OFFSET))
        
        # Créer l'en-tête du bloc (32 bytes)
        # Magic: "UF2\n" (0x0A324655)
        printf "UF2\n" > /tmp/uf2_header.bin
        
        # Flags: 0x00002000 (file container)
        printf "\x00\x00\x20\x00" >> /tmp/uf2_header.bin
        
        # Address dans la flash (little-endian, 4 bytes)
        printf "%08x" $FLASH_ADDR | sed 's/\(..\)\(..\)\(..\)\(..\)/\4\3\2\1/' | xxd -r -p >> /tmp/uf2_header.bin
        
        # Taille des données (little-endian, 4 bytes)
        printf "%08x" $BLOCK_DATA_SIZE | sed 's/\(..\)\(..\)\(..\)\(..\)/\4\3\2\1/' | xxd -r -p >> /tmp/uf2_header.bin
        
        # Numéro de bloc (little-endian, 4 bytes)
        printf "%08x" $BLOCK_NUM | sed 's/\(..\)\(..\)\(..\)\(..\)/\4\3\2\1/' | xxd -r -p >> /tmp/uf2_header.bin
        
        # Nombre total de blocs (little-endian, 4 bytes)
        printf "%08x" $NUM_BLOCKS | sed 's/\(..\)\(..\)\(..\)\(..\)/\4\3\2\1/' | xxd -r -p >> /tmp/uf2_header.bin
        
        # Famille ID: 0x00000000 (generic) - ESP32 utilise generic
        printf "\x00\x00\x00\x00" >> /tmp/uf2_header.bin
        
        # Padding (8 bytes)
        printf "\x00\x00\x00\x00\x00\x00\x00\x00" >> /tmp/uf2_header.bin
        
        # Extraire les données du fichier binaire pour ce bloc
        dd if="$MERGED_BIN" of=/tmp/uf2_data.bin bs=1 skip=$OFFSET count=$BLOCK_DATA_SIZE 2>/dev/null
        
        # Remplir avec des zéros si nécessaire pour atteindre 476 bytes
        PADDING_SIZE=$((DATA_PER_BLOCK - BLOCK_DATA_SIZE))
        if [ $PADDING_SIZE -gt 0 ]; then
            dd if=/dev/zero of=/tmp/uf2_padding.bin bs=1 count=$PADDING_SIZE 2>/dev/null
            cat /tmp/uf2_data.bin /tmp/uf2_padding.bin > /tmp/uf2_data_full.bin
        else
            cp /tmp/uf2_data.bin /tmp/uf2_data_full.bin
        fi
        
        # Footer: 0x0AB16F30 (magic end)
        printf "\x30\x6f\xb1\x0a" > /tmp/uf2_footer.bin
        
        # Assembler le bloc complet (32 + 476 + 4 = 512 bytes)
        cat /tmp/uf2_header.bin /tmp/uf2_data_full.bin /tmp/uf2_footer.bin >> "$UF2_FILE"
        
        OFFSET=$((OFFSET + BLOCK_DATA_SIZE))
        BLOCK_NUM=$((BLOCK_NUM + 1))
    done
    
    # Nettoyer les fichiers temporaires
    rm -f /tmp/uf2_*.bin
    
    if [ -f "$UF2_FILE" ]; then
        if [[ "$OSTYPE" == "darwin"* ]]; then
            UF2_SIZE=$(stat -f%z "$UF2_FILE" 2>/dev/null)
        else
            UF2_SIZE=$(stat -c%s "$UF2_FILE" 2>/dev/null)
        fi
        
        # Formater la taille de manière lisible
        if command -v numfmt &> /dev/null; then
            UF2_SIZE_H=$(numfmt --to=iec-i --suffix=B $UF2_SIZE 2>/dev/null)
        else
            # Fallback simple
            if [ $UF2_SIZE -gt 1048576 ]; then
                UF2_SIZE_H=$(printf "%.1f MB" $(echo "scale=1; $UF2_SIZE / 1048576" | bc))
            elif [ $UF2_SIZE -gt 1024 ]; then
                UF2_SIZE_H=$(printf "%.1f KB" $(echo "scale=1; $UF2_SIZE / 1024" | bc))
            else
                UF2_SIZE_H="${UF2_SIZE} bytes"
            fi
        fi
        
        echo "   ✅ Fichier UF2 généré: $UF2_FILE"
        echo "   📊 Taille: $UF2_SIZE_H"
        echo ""
        echo "   📋 Instructions pour upload:"
        echo "   1. Mettre le XIAO en mode bootloader (double-clic rapide sur RESET)"
        echo "   2. Le XIAO apparaîtra comme un disque USB"
        echo "   3. Glisser-déposer le fichier $UF2_FILE sur le disque"
        echo "   4. Le XIAO redémarrera automatiquement"
    else
        echo "   ❌ Le fichier UF2 n'a pas été généré"
        return 1
    fi
}

# Fonction de build (compilation + stockage binaire)
build_binary() {
    echo "🔨 Compilation et stockage du binaire..."
    echo "   📁 Sketch: $SKETCH_PATH"
    echo "   📋 Board: $BOARD"
    echo "   📦 Dossier de sortie: $REPO_DIR/bin"
    
    # Créer le dossier bin s'il n'existe pas
    mkdir -p "$REPO_DIR/bin"
    
    if command -v arduino-cli &> /dev/null; then
        echo "   Utilisation d'arduino-cli..."
        arduino-cli compile --fqbn "$BOARD" --output-dir "$REPO_DIR/bin" "$SKETCH_PATH"
        echo "   ✅ Binaire compilé et stocké dans bin/"
        echo "   📁 Fichiers créés:"
        ls -la "$REPO_DIR/bin/" 2>/dev/null || echo "   📁 Aucun fichier trouvé"
        
        # Générer automatiquement le fichier UF2
        echo ""
        generate_uf2
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
    
    # Trouver le port série
    PORT=$(ls /dev/cu.usbserial-* /dev/cu.usbmodem* /dev/cu.SLAB_USBtoUART* 2>/dev/null | head -1)
    if [ -z "$PORT" ]; then
        echo "   ❌ Aucun port série trouvé"
        echo "   📝 Ports disponibles:"
        ls /dev/cu.* 2>/dev/null | head -5 || echo "   📝 Aucun port trouvé"
        echo "   📝 Vérifiez que l'ESP32 est connecté"
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
        # Trouver le port série (plusieurs patterns possibles)
        PORT=$(ls /dev/cu.usbserial-* /dev/cu.usbmodem* /dev/cu.SLAB_USBtoUART* 2>/dev/null | head -1)
        if [ -z "$PORT" ]; then
            echo "   ❌ Aucun port série trouvé"
            echo "   📝 Ports disponibles:"
            ls /dev/cu.* 2>/dev/null | head -5 || echo "   📝 Aucun port trouvé"
            echo "   📝 Vérifiez que l'ESP32 est connecté"
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
    case "${1:-help}" in
        "sync")
            echo "🚀 ESP32Server - Synchronisation"
            echo "================================"
            sync_files
            clean_cache
            echo ""
            echo "✅ Synchronisation terminée !"
            echo "📝 Maintenant compile et upload dans l'IDE Arduino"
            ;;
               "compile")
                   echo "🚀 ESP32Server - Synchronisation + Compilation"
                   echo "================================================"
                   sync_files
                   clean_cache
                   compile_sketch
                   echo ""
                   echo "✅ Compilation terminée !"
                   echo "📝 Maintenant upload dans l'IDE Arduino"
                   ;;
               "build")
                   echo "🚀 ESP32Server - Synchronisation + Compilation + Stockage"
                   echo "========================================================"
                   sync_files
                   clean_cache
                   build_binary
                   echo ""
                   echo "✅ Build terminé !"
                   echo "📦 Binaire stocké dans bin/"
                   ;;
               "upload")
                   echo "🚀 ESP32Server - Synchronisation + Compilation + Upload"
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
               "monitor")
                   echo "🚀 ESP32Server - Moniteur série"
                   echo "============================="
                   monitor_serial
                   ;;
        "all")
            echo "🚀 ESP32Server - TOUT FAIRE (Sync + Compile + Upload + Test)"
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
            echo "🚀 ESP32Server - Nettoyage du cache"
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