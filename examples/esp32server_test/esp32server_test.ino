// ============================================================================
// ESP32Server - Test de compilation sans debug
// ============================================================================
// Ce sketch teste la compilation sans les macros de debug définies
// Il devrait compiler sans erreur

// IMPORTANT: Les macros de debug sont définies AVANT Esp32Server.h
// Esp32Server.h inclut automatiquement esp32server_debug.h
// Pas de #define = debug désactivé (zéro overhead)

#include "Esp32Server.h"
#include <Preferences.h>

// Instance globale
Esp32Server server;

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32Server - Test de compilation");
    
    // Initialiser le serveur
    server.begin();
    
    Serial.println("✅ Compilation OK - Debug désactivé");
}

void loop() {
    // Le serveur gère automatiquement les requêtes
    delay(10);
}
