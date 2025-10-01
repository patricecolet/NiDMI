/*
 * ESP32Server Debug - Sketch de debug pour le grisage des pins
 * 
 * Ce sketch active tous les debugs nécessaires pour diagnostiquer
 * le problème du grisage automatique des pins I2C/SPI.
 * 
 * Debug activés :
 * - NETWORK : Pour voir la communication serveur
 * - WEBSOCKET : Pour voir les messages WebSocket
 * - API : Pour voir les appels API
 * - CACHE : Pour voir la gestion du cache NVS
 * 
 * Usage :
 * 1. Uploader ce sketch
 * 2. Ouvrir le moniteur série (115200 baud)
 * 3. Ouvrir http://192.168.4.1 dans Firefox
 * 4. Ouvrir la console Firefox (F12)
 * 5. Cliquer sur SDA
 * 6. Comparer les logs série et console
 */

// ============================================================================
// ACTIVATION DU DEBUG (DOIT ÊTRE AVANT LES INCLUDES!)
// ============================================================================
#define ESP32SERVER_DEBUG_NETWORK 1    // Debug réseau et serveur
#define ESP32SERVER_DEBUG_WEBSOCKET 1  // Debug WebSocket
#define ESP32SERVER_DEBUG_API 1        // Debug API
#define ESP32SERVER_DEBUG_CACHE 1      // Debug cache NVS

// IMPORTANT: Les macros de debug sont définies AVANT Esp32Server.h
// Esp32Server.h inclut automatiquement esp32server_debug.h
#include "Esp32Server.h"
#include <Preferences.h>

void setup() {
    Serial.begin(115200);
    delay(100);
    
    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║  ESP32Server - Debug Mode (Grisage Pins I2C/SPI)      ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
    Serial.println("📋 Debug activés:");
    Serial.println("   ✅ NETWORK   - Communication serveur");
    Serial.println("   ✅ WEBSOCKET - Messages WebSocket");
    Serial.println("   ✅ API       - Appels API");
    Serial.println("   ✅ CACHE     - Gestion cache NVS");
    Serial.println();
    Serial.println("🎯 Test à effectuer:");
    Serial.println("   1. Ouvrir http://192.168.4.1 dans Firefox");
    Serial.println("   2. Ouvrir console Firefox (F12)");
    Serial.println("   3. Cliquer sur SDA");
    Serial.println("   4. Vérifier logs série + console");
    Serial.println();
    Serial.println("══════════════════════════════════════════════════════════");
    Serial.println();
    
    // Initialisation automatique
    esp32server.begin();
    
    Serial.println();
    Serial.println("✅ Serveur prêt - En attente de connexion...");
    Serial.println();
}

void loop() {
    // Traitement automatique
    esp32server.loop();
}
