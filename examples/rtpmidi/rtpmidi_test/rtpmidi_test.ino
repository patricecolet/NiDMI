/*
 * Test RTP-MIDI simple pour ESP32
 * 
 * Ce sketch teste RTP-MIDI sans interface web, juste pour vérifier que ça marche.
 * 
 * Fonctionnalités :
 * - Point d'accès Wi-Fi : "ESP32-RTP-MIDI" (mot de passe : "esp32pass")
 * - RTP-MIDI activé par défaut sur STA
 * - Envoi de notes de test toutes les 3 secondes (même sans connexion détectée)
 * 
 * Pour tester :
 * 1. Connectez votre Mac au même réseau Wi-Fi que l'ESP32
 * 2. L'ESP32 devrait apparaître dans les périphériques MIDI de votre Mac
 * 3. Ouvrez un séquenceur (Logic, GarageBand, etc.) et vérifiez que les notes arrivent
 */

#include <Esp32Server.h>
#include <Preferences.h>

// Instance du serveur ESP32
Esp32Server srv;
Esp32Server& esp32Server = srv;

void setup(){
    Serial.begin(115200);
    while(!Serial){ delay(10); }
    Serial.println("=== ESP32 RTP-MIDI Test ===");
    
    // Configuration du point d'accès Wi-Fi
    const char* apSsid = "ESP32-RTP-MIDI";
    const char* apPass = "esp32pass";
    const char* host = "esp32rtpmidi";
    
    // Configuration du réseau STA
    const char* staSsid = "manca";
    const char* staPass = "manca2022";
    srv.connectSta(staSsid, staPass);
    
    // Attendre la connexion STA
    Serial.println("Attente de la connexion Wi-Fi STA...");
    int attempts = 0;
    while(WiFi.status() != WL_CONNECTED && attempts < 20){
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if(WiFi.status() == WL_CONNECTED){
        Serial.println();
        Serial.print("STA connecté ! IP: "); Serial.println(WiFi.localIP());
    } else {
        Serial.println();
        Serial.println("Échec de la connexion STA");
    }

    // Démarrage du serveur web
    srv.begin(apSsid, apPass, host);
    
    // ========================================
    // ACTIVATION RTP-MIDI PAR DÉFAUT
    // ========================================
    Serial.println("Activation de RTP-MIDI...");
    
    // Initialiser RTP-MIDI seulement si STA est connecté
    if(WiFi.status() == WL_CONNECTED){
        // Initialiser RTP-MIDI
        esp32Server.rtpMidi().begin("ESP32-Test");
        Serial.println("RTP-MIDI initialisé: ESP32-Test");
        Serial.println("Votre ESP32 devrait maintenant apparaître dans les périphériques MIDI de votre Mac/PC");
        Serial.println("Assurez-vous que votre Mac est sur le même réseau Wi-Fi que l'ESP32");
    } else {
        Serial.println("RTP-MIDI non initialisé - STA non connecté");
    }
    
    // Affichage des informations
    Serial.print("AP SSID: "); Serial.println(apSsid);
    Serial.print("AP PASS: "); Serial.println(apPass);
    Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
    Serial.print("Interface web: http://"); Serial.print(host); Serial.println(".local/");
}

void loop(){
    static uint32_t lastNote = 0;
    static uint32_t lastDisplay = 0;
    static bool rtpInitialized = false;
    uint32_t now = millis();
    
    // Initialiser RTP-MIDI si pas encore fait et STA connecté
    if(!rtpInitialized && WiFi.status() == WL_CONNECTED){
        esp32Server.rtpMidi().begin("ESP32-Test");
        Serial.println("RTP-MIDI initialisé: ESP32-Test");
        Serial.println("Votre ESP32 devrait maintenant apparaître dans les périphériques MIDI de votre Mac/PC");
        rtpInitialized = true;
    }
    
    // Affichage périodique des informations réseau
    if(now - lastDisplay > 5000){
        lastDisplay = now;
        //Serial.print("AP IP: "); Serial.print(WiFi.softAPIP());
        //Serial.print("  STA IP: "); Serial.println(WiFi.localIP());
        
        if(WiFi.status() == WL_CONNECTED){
            //Serial.println("Wi-Fi STA connecté - RTP-MIDI disponible");
        } else {
            Serial.println("Wi-Fi STA non connecté - RTP-MIDI indisponible");
        }
        
        if(esp32Server.rtpMidi().isConnected()){
            //Serial.println("RTP-MIDI connecté !");
        } else {
            Serial.println("RTP-MIDI en attente de connexion...");
        }
    }
    
    // Envoi de notes de test toutes les 3 secondes (MÊME SANS CONNEXION DÉTECTÉE)
    if(now - lastNote > 3000){
        lastNote = now;
        
        // Envoyer une note de test même si la connexion n'est pas détectée
        // Cela permet de tester si RTP-MIDI fonctionne
        esp32Server.rtpMidi().sendNoteOn(1, 60, 100);
        //Serial.println("Note envoyée via RTP-MIDI (test forcé)");
        delay(100);
        esp32Server.rtpMidi().sendNoteOff(1, 60, 0);
        
        // Afficher le statut de connexion
        if(esp32Server.rtpMidi().isConnected()){
            Serial.println("  -> Connexion RTP-MIDI détectée");
        } else {
            //Serial.println("  -> Connexion RTP-MIDI non détectée (mais note envoyée)");
        }
    }
    
    // Mise à jour RTP-MIDI (obligatoire)
    esp32Server.rtpMidi().update();
}