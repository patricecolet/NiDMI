/*
  ESP32Server Debug Optimized Example
  
  Ce sketch montre comment utiliser le système de debug ultra-optimisé.
  
  AVANT de compiler, décommentez les #define ci-dessous pour activer
  le debug des modules que vous voulez :
  
  #define ESP32SERVER_DEBUG_OSC 1        // Debug OSC
  #define ESP32SERVER_DEBUG_NETWORK 1     // Debug Network  
  #define ESP32SERVER_DEBUG_COMPONENTS 1  // Debug Components
  #define ESP32SERVER_DEBUG_WEBSOCKET 1  // Debug WebSocket
  #define ESP32SERVER_DEBUG_PINS 1       // Debug Pins
  #define ESP32SERVER_DEBUG_CACHE 1      // Debug Cache
  #define ESP32SERVER_DEBUG_RTPMIDI 1     // Debug RTP-MIDI
  #define ESP32SERVER_DEBUG_API 1         // Debug API
  
  Si aucun #define n'est activé, le système compile SANS AUCUN
  code de debug (zéro overhead) !
*/

// ============================================================================
// CONFIGURATION DU DEBUG - DÉCOMMENTEZ POUR ACTIVER
// ============================================================================

// Décommentez les lignes ci-dessous pour activer le debug des modules souhaités
// #define ESP32SERVER_DEBUG_OSC 1        // Debug OSC
// #define ESP32SERVER_DEBUG_NETWORK 1     // Debug Network  
// #define ESP32SERVER_DEBUG_COMPONENTS 1  // Debug Components
// #define ESP32SERVER_DEBUG_WEBSOCKET 1  // Debug WebSocket
// #define ESP32SERVER_DEBUG_PINS 1       // Debug Pins
// #define ESP32SERVER_DEBUG_CACHE 1      // Debug Cache
// #define ESP32SERVER_DEBUG_RTPMIDI 1     // Debug RTP-MIDI
// #define ESP32SERVER_DEBUG_API 1         // Debug API

// ============================================================================
// INCLUDE DU SYSTÈME DE DEBUG
// ============================================================================

#include "esp32server_debug.h"  // Doit être inclus AVANT Esp32Server.h

// ============================================================================
// INCLUDE DE LA BIBLIOTHÈQUE PRINCIPALE
// ============================================================================

#include <Esp32Server.h>

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32Server Debug Optimized Example ===");
  Serial.println("Système de debug ultra-optimisé");
  Serial.println("Par défaut: complètement silencieux");
  Serial.println("Décommentez les #define pour activer le debug");
  Serial.println("==========================================\n");
  
  // Initialisation du serveur ESP32
  esp32server_begin();
  
  Serial.println("Serveur démarré !");
  Serial.println("- Interface web: http://myesp32.local/");
  Serial.println("- Ou utilisez l'IP affichée ci-dessus");
  Serial.println("\nPour activer le debug, décommentez les #define en haut du fichier");
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
  esp32server_loop();
  delay(10);
}

// ============================================================================
// NOTES D'UTILISATION
// ============================================================================
/*
  SYSTÈME DE DEBUG ULTRA-OPTIMISÉ :
  
  1. PAR DÉFAUT : Complètement silencieux (zéro overhead)
  2. ACTIVATION : Décommentez les #define en haut du fichier
  3. COMPILATION : Le code de debug n'est compilé QUE si activé
  4. MÉMOIRE : Gain de ~5KB par rapport à l'ancien système
  
  EXEMPLES D'ACTIVATION :
  
  Pour debug OSC uniquement :
    #define ESP32SERVER_DEBUG_OSC 1
  
  Pour debug Network et Components :
    #define ESP32SERVER_DEBUG_NETWORK 1
    #define ESP32SERVER_DEBUG_COMPONENTS 1
  
  Pour debug complet :
    #define ESP32SERVER_DEBUG_OSC 1
    #define ESP32SERVER_DEBUG_NETWORK 1
    #define ESP32SERVER_DEBUG_COMPONENTS 1
    #define ESP32SERVER_DEBUG_WEBSOCKET 1
    #define ESP32SERVER_DEBUG_PINS 1
    #define ESP32SERVER_DEBUG_CACHE 1
    #define ESP32SERVER_DEBUG_RTPMIDI 1
    #define ESP32SERVER_DEBUG_API 1
*/
