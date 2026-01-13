/*
 * NiDMI Basic - ESP32-C3/S3 (XIAO)
 * 
 * Sketch universel pour ESP32-C3 et ESP32-S3 (XIAO)
 * Le MCU est détecté automatiquement
 * 
 * Ce sketch utilise l'architecture optimisée avec ComponentManager.
 * 
 * Fonctionnalités :
 * - Détection automatique du MCU (ESP32-C3 ou ESP32-S3)
 * - Interface web pour configuration des pins
 * - RTP-MIDI automatique
 * - Gestion optimisée des composants
 * - OSC (Open Sound Control)
 * - Touch pins (ESP32-S3 uniquement)
 * 
 * Pins disponibles :
 * - ESP32-C3 : A0-A2, D0-D10 (A3 n'existe pas)
 * - ESP32-S3 : A0-A4, D0-D9 (toutes les touch pins sont analogiques)
 * 
 * Usage :
 * 1. Sélectionner le board : XIAO_ESP32C3 ou XIAO_ESP32S3 dans Arduino IDE
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
 * Option NVS Clear :
 * - Décommentez la ligne CLEAR_NVS ci-dessous pour forcer le reset
 * - Utile si des anciens réglages persistent
 */

#include "NiDMIServer.h"
#include <Preferences.h>

// Décommentez la ligne suivante pour forcer le nettoyage NVS
// #define CLEAR_NVS

void setup() {
    Serial.begin(115200);
    delay(100);
    
    #ifdef CLEAR_NVS
    Serial.println("[NiDMI] Clearing NVS...");
    Preferences preferences;
    preferences.begin("nidmi", false);
    preferences.clear();
    preferences.end();
    Serial.println("[NiDMI] NVS cleared!");
    #endif
    
    // Initialisation automatique (détection MCU automatique)
    nidmi.begin();
    
    // Le système est maintenant prêt :
    // - WiFi AP "esp32rtpmidi" actif
    // - Interface web sur http://192.168.4.1
    // - RTP-MIDI initialisé automatiquement
    // - ComponentManager prêt à recevoir des configurations
}

void loop() {
    // Traitement automatique
    nidmi.loop();
    
    // Le ComponentManager gère :
    // - Lecture des potentiomètres
    // - Anti-rebond des boutons
    // - Envoi MIDI automatique
    // - Rechargement des configurations
}
