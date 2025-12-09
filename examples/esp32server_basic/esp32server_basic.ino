/*
 * ESP32Server Basic - Sketch UNIQUE et PRINCIPAL
 * 
 * ⚠️ IMPORTANT : Ce sketch est le seul sketch recommandé.
 * Les autres sketches de debug ne sont plus utilisés.
 * Modifier ce sketch uniquement si nécessaire pour avancer.
 * 
 * Ce sketch utilise l'architecture optimisée avec ComponentManager.
 * 
 * Fonctionnalités :
 * - Détection automatique du MCU (ESP32-C3/S3)
 * - Interface web pour configuration des pins
 * - RTP-MIDI automatique
 * - Gestion optimisée des composants
 * - OSC (Open Sound Control)
 * 
 * Usage :
 * 1. Uploader ce sketch
 * 2. Se connecter au WiFi "esp32rtpmidi" (mot de passe: "esp32pass")
 * 3. Ouvrir http://192.168.4.1 dans Firefox (recommandé)
 * 4. Configurer les pins via l'interface web
 * 5. Les composants envoient automatiquement du MIDI
 * 
 * 🌐 Navigateur recommandé : Firefox
 * - Firefox fonctionne immédiatement sans configuration
 * - Interface web compatible avec tous les navigateurs
 * - Brave/Chrome fonctionnent aussi pour la configuration
 * 
 * 🎵 Web MIDI (à venir)
 * - Web MIDI n'est pas encore implémenté dans l'interface actuelle
 * - Une page de test Web MIDI sera disponible sur GitHub (HTTPS)
 * - Cette page permettra de tester Web MIDI avec l'ESP32
 * - Firefox sera recommandé pour cette fonctionnalité future
 * 
 * Option NVS Clear :
 * - Décommentez la ligne CLEAR_NVS ci-dessous pour forcer le reset
 * - Utile si des anciens réglages persistent
 */

// ============================================================================
// ACTIVATION DU DEBUG (DOIT ÊTRE AVANT LES INCLUDES!)
// ============================================================================
// Décommentez les lignes suivantes pour activer le debug
// Debug désactivé (système de logs retiré)

#include "Esp32Server.h"
#include <Preferences.h>

// Décommentez la ligne suivante pour forcer le nettoyage NVS
// #define CLEAR_NVS

void setup() {
    Serial.begin(115200);
    delay(100);
    
    #ifdef CLEAR_NVS
    Serial.println("[ESP32Server] Clearing NVS...");
    Preferences preferences;
    preferences.begin("esp32server", false);
    preferences.clear();
    preferences.end();
    Serial.println("[ESP32Server] NVS cleared!");
    #endif
    
    // Initialisation automatique
    esp32server.begin();
    
    // Le système est maintenant prêt :
    // - WiFi AP "esp32rtpmidi" actif
    // - Interface web sur http://192.168.4.1
    // - RTP-MIDI initialisé automatiquement
    // - ComponentManager prêt à recevoir des configurations
}

void loop() {
    // Traitement automatique
    esp32server.loop();
    
    // Le ComponentManager gère :
    // - Lecture des potentiomètres
    // - Anti-rebond des boutons
    // - Envoi MIDI automatique
    // - Rechargement des configurations
}