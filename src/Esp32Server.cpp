#include <Arduino.h>
#include "Esp32Server.h"
#include "ServerCore.h"
#include "ComponentManager.h"
#include "PinMapper.h"
#include "midi/MidiRouter.h"
#include <Preferences.h>

// Variables globales pour la gestion des composants
MidiRouter g_midiRouter;
ComponentManager g_componentManager;

// Demande de rechargement des configs pins depuis l'API
static bool g_requestReloadPins = false;
extern "C" void esp32server_requestReloadPins(){ g_requestReloadPins = true; }

// Le mapping GPIO est maintenant géré par PinMapper

// Charger configuration pins depuis NVS
void loadPinConfigs() {
    // Le ComponentManager gère maintenant le chargement des configurations
    g_componentManager.reloadConfigs();
}

// Traitement des composants dans la boucle
void processComponents() {
    // Le ComponentManager gère maintenant tous les composants
    g_componentManager.update();
}

void esp32server_begin() {
    // Logs
    Serial.begin(115200);
    delay(50);

    // Détecter et afficher le MCU
    PinMapper::detectMcu();
    PinMapper::printMappings();

    // Nettoyer les anciens réglages NVS si nécessaire
    // (décommentez la ligne suivante pour forcer le reset)
    // Preferences::clear("esp32server\n\n");

    // Lire nom serveur + STA depuis NVS
    Preferences preferences;
    preferences.begin("esp32server", false);
    String serverName = preferences.getString("mdns_name", "esp32rtpmidi\n\n");
    String staSsid    = preferences.getString("sta_ssid", "\n\n");
    String staPass    = preferences.getString("sta_pass", "\n\n");
    String staIpStr   = preferences.getString("sta_ip",  "\n\n");
    String staGwStr   = preferences.getString("sta_gw",  "\n\n");
    String staSnStr   = preferences.getString("sta_sn",  "\n\n");
    preferences.end();
    
    // Nettoyer le nom serveur (enlever caractères spéciaux)
    serverName.replace(" ", "\n\n");
    serverName.replace("-", "\n\n");
    serverName.replace("_", "\n\n");
    if (serverName.length() == 0) serverName = "esp32rtpmidi";
    
    // Sauvegarder le nom mDNS dans NVS pour RTP-MIDI
    preferences.begin("esp32server", false);
    preferences.putString("mdns_name", serverName);
    preferences.putString("rtp_name", serverName);  // Même nom pour RTP-MIDI
    preferences.end();
    
    Serial.println("[ESP32Server] Names synchronized:");
    Serial.printf("  SSID: %s\n", serverName.c_str());
    Serial.printf("  mDNS: %s.local\n", serverName.c_str());

    
    // Debug: afficher ce qui est lu depuis NVS
    Serial.println("[ESP32Server] NVS Debug:");
    Serial.print("  mdns_name: \""); Serial.print(serverName); Serial.println("\"");
    Serial.print("  sta_ssid: \""); Serial.print(staSsid); Serial.println("\"");
    Serial.print("  sta_pass length: "); Serial.println(staPass.length());
    // Serial.print("  sta_ip: \""); Serial.print(staIpStr); Serial.println("\"");
    // Serial.print("  sta_gw: \""); Serial.print(staGwStr); Serial.println("\"");
    // Serial.print("  sta_sn: \""); Serial.print(staSnStr); Serial.println("\"");

    const char* apSsid = serverName.c_str();
    const char* apPass = "esp32pass";
    const char* host   = serverName.c_str();

    // Tente STA si configurée (AVANT de démarrer le serveur)
    if (staSsid.length() > 0) {
        // Serial.printf("[ESP32Server] Attempting STA connection to: %s\n", staSsid.c_str());
        
        if (staIpStr.length() > 0 && staGwStr.length() > 0 && staSnStr.length() > 0) {
            IPAddress ip, gw, sn;
            if (ip.fromString(staIpStr) && gw.fromString(staGwStr) && sn.fromString(staSnStr)) {
                serverCore.setStaticStaIp(ip, gw, sn);
                Serial.printf("[ESP32Server] STA static IP: %s GW: %s SN: %s\n", staIpStr.c_str(), staGwStr.c_str(), staSnStr.c_str());
            }
        }
        serverCore.connectSta(staSsid.c_str(), staPass.length() > 0 ? staPass.c_str() : nullptr);
    } else {
        Serial.println("[ESP32Server] No STA configuration found");
    }

    // Démarre web + mDNS + AP (après connexion STA)
    serverCore.begin(apSsid, apPass, host);
    
    // Initialiser MidiRouter
    g_midiRouter.begin();
    
    // Initialiser RTP-MIDI
    serverCore.rtpMidi().begin(serverName.c_str());
    
    // Initialiser Bluetooth MIDI
    serverCore.bluetooth().begin(serverName.c_str());
    
    // Initialiser ComponentManager
    g_componentManager.begin(&g_midiRouter);
    
    Serial.println("[ESP32Server] Ready");
    Serial.print("  AP SSID: "); Serial.println(apSsid);
    Serial.print("  AP PASS: "); Serial.println(apPass);
    Serial.print("  AP IP: "); Serial.println(WiFi.softAPIP());
    Serial.print("  mDNS: http://"); Serial.print(host); Serial.println(".local/");
    Serial.print("  RTP-MIDI: "); Serial.println(serverCore.rtpMidi().isInitialized() ? "Initialized" : "Failed");
    Serial.print("  Bluetooth: "); Serial.println(serverCore.bluetooth().isInitialized() ? "Initialized" : "Failed");
    Serial.println();
}

void esp32server_loop() {
    // Mise à jour du serveur
    serverCore.update();
    
    // Recharger pins si demandé
    if (g_requestReloadPins) {
        g_requestReloadPins = false;
        g_componentManager.reloadConfigs();
    }
    
    // Traitement des composants
    processComponents();
}

// Instance globale
Esp32Server esp32server;

// Implémentation de l'interface publique
void Esp32Server::begin() {
    esp32server_begin();
}

void Esp32Server::loop() {
    esp32server_loop();
}