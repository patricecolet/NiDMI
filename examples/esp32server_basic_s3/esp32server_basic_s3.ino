/*
 * ESP32Server Basic - ESP32-S3 (XIAO)
 * 
 * Sketch spécifique pour ESP32-S3 (XIAO_ESP32S3)
 * 
 * ⚠️ IMPORTANT : Ce sketch est pour ESP32-S3 uniquement
 * Pour ESP32-C3, utilisez esp32server_basic_c3
 * 
 * Ce sketch utilise l'architecture optimisée avec ComponentManager.
 * 
 * Fonctionnalités :
 * - Détection automatique du MCU (ESP32-S3)
 * - Interface web pour configuration des pins
 * - RTP-MIDI automatique
 * - Gestion optimisée des composants
 * - OSC (Open Sound Control)
 * - Touch pins (fonctionnalité à venir)
 * - USB MIDI (fonctionnalité à venir)
 * 
 * Pins disponibles sur ESP32-S3 (XIAO) :
 * - Analogiques : A0, A1, A2, A3, A4 (toutes les touch pins sont analogiques)
 * - Digitales : D0-D9
 * - Touch pins : D0-D9 (GPIO1-10) - toutes sont analogiques
 * - I2C : SDA (D3/GPIO4), SCL (D4/GPIO5)
 * - SPI : MOSI (D6/GPIO7), MISO (D5/GPIO6), SCK (D7/GPIO8)
 * - UART : TX (GPIO43), RX (GPIO44) - pas analogiques
 * 
 * Usage :
 * 1. Sélectionner le board : XIAO_ESP32S3 dans Arduino IDE
 * 2. Uploader ce sketch
 * 3. Se connecter au WiFi "esp32rtpmidi" (mot de passe: "esp32pass")
 * 4. Ouvrir http://192.168.4.1 dans Firefox (recommandé)
 * 5. Configurer les pins via l'interface web
 * 6. Les composants envoient automatiquement du MIDI
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

