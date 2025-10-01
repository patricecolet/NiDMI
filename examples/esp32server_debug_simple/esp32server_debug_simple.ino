// ============================================================================
// ESP32Server - Debug Simple
// ============================================================================
// Ce sketch teste le système de debug avec un code minimal

// ACTIVATION DU DEBUG (DOIT ÊTRE AVANT LES INCLUDES!)
// ============================================================================
#define ESP32SERVER_DEBUG_NETWORK 1    // Debug réseau et serveur
#define ESP32SERVER_DEBUG_WEBSOCKET 1  // Debug WebSocket
#define ESP32SERVER_DEBUG_API 1        // Debug API
#define ESP32SERVER_DEBUG_CACHE 1      // Debug cache NVS

#include "Esp32Server.h"
#include <Preferences.h>

// Instance globale
Esp32Server server;

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32Server - Debug Simple");
    
    // Test des macros de debug
    debug_network("🔧 Test debug_network - Le système de debug fonctionne !\n");
    debug_websocket("🔧 Test debug_websocket - Le système de debug fonctionne !\n");
    debug_api("🔧 Test debug_api - Le système de debug fonctionne !\n");
    debug_cache("🔧 Test debug_cache - Le système de debug fonctionne !\n");
    
    Serial.println("✅ Tous les tests de debug sont passés !");
    Serial.println("🎯 Le système de debug fonctionne maintenant !");
    
    // Initialiser le serveur
    server.begin();
    
    Serial.println("🚀 Serveur initialisé avec debug activé");
}

void loop() {
    // Le serveur gère automatiquement les requêtes
    delay(10);
}
