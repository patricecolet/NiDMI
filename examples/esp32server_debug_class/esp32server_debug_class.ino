// ============================================================================
// ESP32Server - Debug avec classe DebugManager
// ============================================================================
// Ce sketch montre comment utiliser la classe DebugManager pour contrôler
// le debug depuis le sketch avec des options configurables.

#include "Esp32Server.h"
#include "DebugManager.h"
#include <Preferences.h>

// Instance globale du gestionnaire de debug
DebugManager debugManager;

// Instance globale du serveur
Esp32Server server;

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32Server - Debug avec classe DebugManager");
    
    // Configuration du debug depuis le sketch
    // ======================================
    
    // Option 1: Activer tous les modules de debug
    debugManager.enableAll();
    
    // Option 2: Activer seulement certains modules
    // debugManager.network = true;
    // debugManager.websocket = true;
    // debugManager.api = true;
    
    // Option 3: Configurer le niveau de verbosité
    debugManager.setVerbosity(DebugManager::INFO);
    
    // Initialiser l'instance globale
    g_debug = &debugManager;
    
    // Test des différents types de debug
    debug_network("🔧 Test debug_network - Le système de debug fonctionne !");
    debug_websocket("🔧 Test debug_websocket - Le système de debug fonctionne !");
    debug_api("🔧 Test debug_api - Le système de debug fonctionne !");
    debug_cache("🔧 Test debug_cache - Le système de debug fonctionne !");
    
    // Test des niveaux de verbosité
    debugManager.error("❌ Message d'erreur");
    debugManager.warning("⚠️  Message d'avertissement");
    debugManager.info("ℹ️  Message d'information");
    debugManager.debug("🐛 Message de debug");
    
    Serial.println("✅ Tous les tests de debug sont passés !");
    Serial.println("🎯 Le système de debug avec classe fonctionne !");
    
    // Initialiser le serveur
    server.begin();
    
    Serial.println("🚀 Serveur initialisé avec debug configurable");
}

void loop() {
    // Le serveur gère automatiquement les requêtes
    
    // Exemple: Changer le niveau de debug en runtime
    static unsigned long lastChange = 0;
    if (millis() - lastChange > 10000) { // Toutes les 10 secondes
        lastChange = millis();
        
        // Alterner entre différents niveaux
        static bool toggle = false;
        if (toggle) {
            debugManager.setVerbosity(DebugManager::DEBUG);
            debugManager.info("🔄 Niveau de debug changé à DEBUG");
        } else {
            debugManager.setVerbosity(DebugManager::INFO);
            debugManager.info("🔄 Niveau de debug changé à INFO");
        }
        toggle = !toggle;
    }
    
    delay(10);
}
