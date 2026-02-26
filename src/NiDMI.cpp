#include "NiDMI.h"
#include "server/ServerCore.h"
#include "managers/ComponentManager.h"
#include "utils/PinMapper.h"
#include "midi/MidiRouter.h"
#include "Globals.h"
#include <Preferences.h>

// Variables globales pour la gestion des composants
MidiRouter g_midiRouter;
ComponentManager g_componentManager;

// Demande de rechargement des configs pins depuis l'API
static bool g_requestReloadPins = false;
extern "C" void nidmi_requestReloadPins(){ g_requestReloadPins = true; }

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

// Diagnostic touch au boot : mettre à 1 pour activer, 0 pour désactiver.
#define TOUCH_BOOT_DIAG 1

static void touchDiag(const char* label) {
#if TOUCH_BOOT_DIAG && (defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ARDUINO_ESP32S3_DEV) || defined(ARDUINO_ESP32S3))
    Serial.printf("[TOUCH DIAG] %s: ", label);
    for (int i = 0; i < 5; i++) {
        uint32_t v = touchRead(1); // GPIO1 = T1
        Serial.printf("%lu ", (unsigned long)v);
        delay(50);
    }
    Serial.println();
#else
    (void)label;
#endif
}

void nidmi_begin() {
    Serial.begin(115200);
    delay(50);

    touchDiag("AVANT tout (juste apres Serial)");

    // Détecter et afficher le MCU
    PinMapper::detectMcu();
    PinMapper::printMappings();

    // Nettoyer les anciens réglages NVS si nécessaire
    // (décommentez la ligne suivante pour forcer le reset)
    // Preferences::clear("nidmi\n\n");

    // Lire nom serveur + STA depuis NVS
    Preferences preferences;
    preferences.begin("nidmi", false);
    
    String serverName = preferences.getString("mdns_name", "nidmi");
    
    // Lire STA config une par une pour limiter le nombre de String simultanées
    String staSsid = preferences.getString("sta_ssid", "");
    String staPass = preferences.getString("sta_pass", "");
    String staIpStr = preferences.getString("sta_ip", "");
    String staGwStr = preferences.getString("sta_gw", "");
    String staSnStr = preferences.getString("sta_sn", "");
    
    // Charger l'état des interfaces MIDI depuis NVS
    // bool usbMidiEnabled = preferences.getBool("usbmidi_enabled", true); // Par défaut true
     bool usbMidiEnabled = false; // Par défaut true
    
    preferences.end();
    
    // Nettoyer le nom serveur (supprimer caractères invalides pour SSID WiFi)
    serverName.trim();  // Supprimer espaces en début/fin
    // Supprimer les caractères de contrôle et caractères invalides
    serverName.replace("\n", "");
    serverName.replace("\r", "");
    serverName.replace("\t", "");
    if (serverName.length() == 0) serverName = "nidmi";
    
    // Sauvegarder le nom mDNS dans NVS pour RTP-MIDI
    preferences.begin("nidmi", false);
    preferences.putString("mdns_name", serverName);
    preferences.putString("rtp_name", serverName);  // Même nom pour RTP-MIDI
    preferences.end();
    
    Serial.println("[NiDMI] Names synchronized:");
    Serial.printf("  SSID: %s\n", serverName.c_str());
    Serial.printf("  mDNS: %s.local\n", serverName.c_str());

    const char* apSsid = serverName.c_str();
    const char* apPass = "nidmipass";
    const char* host   = serverName.c_str();

    // Tente STA si configurée (AVANT de démarrer le serveur)
    if (staSsid.length() > 0) {
        if (staIpStr.length() > 0 && staGwStr.length() > 0 && staSnStr.length() > 0) {
            IPAddress ip, gw, sn;
            if (ip.fromString(staIpStr) && gw.fromString(staGwStr) && sn.fromString(staSnStr)) {
                serverCore.setStaticStaIp(ip, gw, sn);
                Serial.printf("[NiDMI] STA static IP: %s GW: %s SN: %s\n", staIpStr.c_str(), staGwStr.c_str(), staSnStr.c_str());
            }
        }
        serverCore.connectSta(staSsid.c_str(), staPass.length() > 0 ? staPass.c_str() : nullptr);
    } else {
        Serial.println("[NiDMI] No STA configuration found");
    }

    touchDiag("AVANT WiFi/serveur");

    // Démarre web + mDNS + AP (après connexion STA)
    serverCore.begin(apSsid, apPass, host);

    touchDiag("APRES WiFi/serveur");
    
    // Appliquer l'état sauvegardé des interfaces MIDI au MidiRouter
    g_midiRouter.enableUsbMidi(usbMidiEnabled);
    
    // Initialiser MidiRouter (qui initialisera USB MIDI si activé et supporté)
    g_midiRouter.begin();
    
    // Initialiser RTP-MIDI
    serverCore.rtpMidi().begin(serverName.c_str());
    
    // Initialiser Bluetooth MIDI
    serverCore.bluetooth().begin(serverName.c_str());
    
    touchDiag("AVANT ComponentManager.begin");

    // Initialiser ComponentManager
    g_componentManager.begin(&g_midiRouter);

    touchDiag("APRES ComponentManager.begin (MuxTask+MidiTask demarres)");
    
    Serial.println("[NiDMI] Ready");
    Serial.print("  AP SSID: "); Serial.println(apSsid);
    Serial.print("  AP PASS: "); Serial.println(apPass);
    Serial.print("  AP IP: "); Serial.println(WiFi.softAPIP());
    Serial.print("  mDNS: http://"); Serial.print(host); Serial.println(".local/");
    Serial.print("  RTP-MIDI: "); Serial.println(serverCore.rtpMidi().isInitialized() ? "Initialized" : "Failed");
    Serial.print("  Bluetooth: "); Serial.println(serverCore.bluetooth().isInitialized() ? "Initialized" : "Failed");
    Serial.println();
}

void nidmi_loop() {
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
NiDMIServer nidmi;

// Implémentation de l'interface publique
void NiDMIServer::begin() {
    nidmi_begin();
}

void NiDMIServer::loop() {
    nidmi_loop();
}
