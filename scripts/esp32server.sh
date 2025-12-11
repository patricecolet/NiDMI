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
SKETCH_PATH="/Users/patricecolet/Documents/Arduino/libraries/esp32server/examples/$SKETCH_NAME"

# Fonction d'aide
show_help() {
    echo "🚀 ESP32Server - Script unifié"
    echo "=============================="
    echo ""
    echo "Usage: ./scripts/esp32server.sh [OPTION] [SKETCH]"
    echo ""
       echo "Options:"
       echo "  sync     - Synchroniser les fichiers seulement"
       echo "  compile  - Synchroniser + compiler"
       echo "  build    - Synchroniser + compiler + stocker le binaire"
       echo "  upload   - Synchroniser + compiler + uploader"
       echo "  monitor  - Ouvrir le moniteur série"
       echo "  all      - Tout faire (sync + compile + upload + test)"
       echo "  clean    - Nettoyer le cache seulement"
       echo "  help     - Afficher cette aide"
    echo ""
    echo "Sketches disponibles:"
    echo "  esp32server_basic (défaut)"
    echo "  esp32server_osc"
    echo "  components_basic"
    echo "  rtpmidi/*"
    echo ""
    echo "Exemples:"
    echo "  ./scripts/esp32server.sh sync     # Juste synchroniser"
    echo "  ./scripts/esp32server.sh compile   # Synchroniser + compiler"
    echo "  ./scripts/esp32server.sh build    # Synchroniser + compiler + stocker binaire"
    echo "  ./scripts/esp32server.sh upload   # Synchroniser + compiler + uploader (sketch par défaut)"
    echo "  ./scripts/esp32server.sh upload esp32server_osc  # Upload sketch OSC"
    echo "  ./scripts/esp32server.sh monitor  # Ouvrir le moniteur série"
    echo "  ./scripts/esp32server.sh all      # Tout faire + test"
    echo "  ./scripts/esp32server.sh clean    # Nettoyer le cache"
    echo ""
}

# Fonction de synchronisation
sync_files() {
    echo "🔄 Synchronisation des fichiers..."
    echo "   📁 Source: $REPO_DIR"
    echo "   📁 Destination: $ARDUINO_LIB_DIR"
    
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