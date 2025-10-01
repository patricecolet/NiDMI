/*
 * ESP32Server - Système de Debug Complet
 * 
 * Ce sketch explique et démontre le système de debug modulaire d'ESP32Server.
 * 
 * SYSTÈME DE DEBUG :
 * - Par défaut : COMPLÈTEMENT SILENCIEUX (aucun message)
 * - Contrôle granulaire : chaque module peut être activé séparément
 * - Niveaux de debug : OFF, ERROR, WARNING, INFO, VERBOSE
 * - Modules disponibles : OSC, WEBSOCKET, PINS, CACHE, NETWORK, COMPONENTS, RTPMIDI, API
 * 
 * UTILISATION :
 * 1. Par défaut : rien ne s'affiche (silencieux)
 * 2. Pour activer : modifier les variables ESP32SERVER_DEBUG_*
 * 3. Pour tester : décommentez les sections ci-dessous
 */

#include <Esp32Server.h>

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== ESP32SERVER DEBUG SYSTEM ===");
    Serial.println("Système de debug modulaire - Contrôle granulaire");
    Serial.println("===============================================");
    
    // ============================================================================
    // CONFIGURATION PAR DÉFAUT (SILENCIEUX)
    // ============================================================================
    Serial.println("\n1. CONFIGURATION PAR DÉFAUT :");
    Serial.println("   - ESP32SERVER_DEBUG_LEVEL = DEBUG_OFF");
    Serial.println("   - Tous les modules = DEBUG_OFF");
    Serial.println("   - Résultat : AUCUN message de debug");
    
    // ============================================================================
    // EXEMPLE 1 : DEBUG OSC UNIQUEMENT
    // ============================================================================
    /*
    Serial.println("\n2. EXEMPLE 1 - DEBUG OSC UNIQUEMENT :");
    ESP32SERVER_DEBUG_LEVEL = DEBUG_VERBOSE;
    ESP32SERVER_DEBUG_MODULES[DEBUG_MODULE_OSC] = DEBUG_VERBOSE;
    // Tous les autres modules restent à DEBUG_OFF
    Serial.println("   - Seuls les messages OSC apparaîtront");
    */
    
    // ============================================================================
    // EXEMPLE 2 : DEBUG NETWORK + COMPONENTS
    // ============================================================================
    /*
    Serial.println("\n3. EXEMPLE 2 - DEBUG NETWORK + COMPONENTS :");
    ESP32SERVER_DEBUG_LEVEL = DEBUG_INFO;
    ESP32SERVER_DEBUG_MODULES[DEBUG_MODULE_NETWORK] = DEBUG_INFO;
    ESP32SERVER_DEBUG_MODULES[DEBUG_MODULE_COMPONENTS] = DEBUG_INFO;
    Serial.println("   - Messages Network et Components apparaîtront");
    */
    
    // ============================================================================
    // EXEMPLE 3 : DEBUG COMPLET (TOUS LES MODULES)
    // ============================================================================
    /*
    Serial.println("\n4. EXEMPLE 3 - DEBUG COMPLET :");
    ESP32SERVER_DEBUG_LEVEL = DEBUG_VERBOSE;
    for (int i = 0; i < DEBUG_MODULE_COUNT; i++) {
        ESP32SERVER_DEBUG_MODULES[i] = DEBUG_VERBOSE;
    }
    Serial.println("   - Tous les messages de debug apparaîtront");
    */
    
    // ============================================================================
    // EXEMPLE 4 : DEBUG ÉTAPES (PROGRESSIF)
    // ============================================================================
    /*
    Serial.println("\n5. EXEMPLE 4 - DEBUG ÉTAPES :");
    Serial.println("   - Étape 1 : Debug Network seulement");
    ESP32SERVER_DEBUG_LEVEL = DEBUG_INFO;
    ESP32SERVER_DEBUG_MODULES[DEBUG_MODULE_NETWORK] = DEBUG_INFO;
    
    // Initialiser ESP32Server
    esp32server_begin();
    
    delay(2000);
    Serial.println("   - Étape 2 : Ajouter debug Components");
    ESP32SERVER_DEBUG_MODULES[DEBUG_MODULE_COMPONENTS] = DEBUG_INFO;
    
    delay(2000);
    Serial.println("   - Étape 3 : Ajouter debug OSC");
    ESP32SERVER_DEBUG_MODULES[DEBUG_MODULE_OSC] = DEBUG_VERBOSE;
    */
    
    // ============================================================================
    // FONCTIONS UTILES
    // ============================================================================
    Serial.println("\n6. FONCTIONS UTILES :");
    Serial.println("   - esp32server_debug_status() : Affiche l'état du debug");
    Serial.println("   - DEBUG_OFF, DEBUG_ERROR, DEBUG_WARNING, DEBUG_INFO, DEBUG_VERBOSE");
    Serial.println("   - DEBUG_MODULE_OSC, DEBUG_MODULE_NETWORK, etc.");
    
    // ============================================================================
    // INITIALISATION (SILENCIEUSE PAR DÉFAUT)
    // ============================================================================
    Serial.println("\n7. INITIALISATION ESP32SERVER :");
    Serial.println("   - Aucun message de debug ne devrait apparaître ci-dessous");
    Serial.println("   - (sauf si vous avez décommenté une configuration ci-dessus)");
    Serial.println("   ----------------------------------------");
    
    // Initialiser ESP32Server (silencieux par défaut)
    esp32server_begin();
    
    Serial.println("   ----------------------------------------");
    Serial.println("   - Initialisation terminée");
    
    // Afficher l'état du debug
    Serial.println("\n8. ÉTAT ACTUEL DU DEBUG :");
    esp32server_debug_status();
    
    Serial.println("\n=== FIN DE LA DÉMONSTRATION ===");
    Serial.println("Pour activer le debug, décommentez une des sections ci-dessus");
    Serial.println("et recompilez le sketch.");
}

void loop() {
    esp32server_loop();
    delay(100);
}
