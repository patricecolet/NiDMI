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

// Demande de rechargement des configs pins depuis l'API (débounce 500 ms pour grouper les sauvegardes séquentielles)
static volatile bool g_requestReloadPins = false;
static unsigned long g_reloadRequestTime = 0;
extern "C" void nidmi_requestReloadPins(){
    g_requestReloadPins = true;
    g_reloadRequestTime = millis();
}

// Redémarrage différé (depuis la loop, pas depuis le handler HTTP — évite de couper la NVS en plein écriture)
static volatile bool g_requestReboot = false;
static unsigned long g_rebootRequestTime = 0;
extern "C" void nidmi_requestReboot(){
    g_rebootRequestTime = millis();
    g_requestReboot = true;
}

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

    // Lire nom serveur + STA depuis NVS (lecture seule d'abord pour éviter crash si NVS abîmée)
    Preferences preferences;
    if (!preferences.begin("nidmi", true)) {
        Serial.println("[NiDMI] ERREUR: ouverture NVS en lecture échouée - NVS peut être corrompue");
        Serial.println("[NiDMI] Utilisation des valeurs par défaut. Flashez nidmi_clear_nvs pour réinitialiser.");
    }
    
    String serverName = preferences.getString("mdns_name", "nidmi");
    String staSsid = preferences.getString("sta_ssid", "");
    String staPass = preferences.getString("sta_pass", "");
    String staIpStr = preferences.getString("sta_ip", "");
    String staGwStr = preferences.getString("sta_gw", "");
    String staSnStr = preferences.getString("sta_sn", "");
    bool touchEnabled = preferences.getBool("touch_enabled", false);
    bool usbMidiEnabled = false;
    
    Serial.println("===== NVS DEBUG (boot) =====");
    Serial.printf("sta_ssid: '%s'\n", staSsid.c_str());
    Serial.printf("sta_pass length: %d\n", (int)staPass.length());
    Serial.printf("sta_ip: '%s' sta_gw: '%s' sta_sn: '%s'\n", staIpStr.c_str(), staGwStr.c_str(), staSnStr.c_str());
    Serial.printf("touch_enabled: %s\n", touchEnabled ? "true" : "false");
    Serial.println("============================");
    
    preferences.end();
    
    // Nettoyer le nom serveur (supprimer caractères invalides pour SSID WiFi)
    serverName.trim();  // Supprimer espaces en début/fin
    // Supprimer les caractères de contrôle et caractères invalides
    serverName.replace("\n", "");
    serverName.replace("\r", "");
    serverName.replace("\t", "");
    if (serverName.length() == 0) serverName = "nidmi";
    
    // Sauvegarder le nom mDNS dans NVS pour RTP-MIDI (seulement si NVS ouvre en écriture)
    if (preferences.begin("nidmi", false)) {
        preferences.putString("mdns_name", serverName);
        preferences.putString("rtp_name", serverName);
        preferences.end();
    }
    
    Serial.println("[NiDMI] Names synchronized:");
    Serial.printf("  SSID: %s\n", serverName.c_str());
    Serial.printf("  mDNS: %s.local\n", serverName.c_str());

    const char* apSsid = serverName.c_str();
    const char* apPass = "nidmipass";
    const char* host   = serverName.c_str();

    touchDiag("AVANT WiFi/serveur");

    // Démarre AP + mode APSTA d'abord
    serverCore.begin(apSsid, apPass, host);

    touchDiag("APRES WiFi/serveur");

    // Tente STA après que le mode APSTA soit configuré
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
    // Redémarrage différé (laisse le temps à la réponse HTTP et à la NVS de se fermer proprement)
    if (g_requestReboot && (millis() - g_rebootRequestTime >= 2000)) {
        ESP.restart();
    }
    
    serverCore.update();
    
    // Recharger pins si demandé (débounce 500 ms pour grouper les sauvegardes séquentielles)
    if (g_requestReloadPins && (millis() - g_reloadRequestTime >= 500)) {
        g_requestReloadPins = false;
        g_componentManager.reloadConfigs();
    }
    
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
