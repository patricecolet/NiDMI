#include "NiDMI.h"
#include "server/ServerCore.h"
#include "managers/ComponentManager.h"
#include "utils/PinMapper.h"
#include "midi/MidiRouter.h"
#include "network/UsbMidiManager.h"
#include "server/WebDebugConsole.h"
#include "Globals.h"
#include <Preferences.h>
#include <WiFi.h>
#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ARDUINO_ESP32S3_DEV) || defined(ARDUINO_ESP32S3)
#include <esp32-hal-tinyusb.h>
#endif

// Variables globales pour la gestion des composants
MidiRouter g_midiRouter;
ComponentManager g_componentManager;

// Configuration STA mise en cache pour permettre une reconnexion automatique
static String g_staSsid;
static String g_staPass;
static String g_staIpStr;
static String g_staGwStr;
static String g_staSnStr;

// Gestion de la reconnexion STA
static unsigned long g_lastStaConnectAttempt = 0;
static const unsigned long STA_RECONNECT_INTERVAL_MS = 10000; // 10 s

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

    // Ne pas laisser la pile WiFi relire/écrire une config STA/AP dans la NVS système
    // (notre SSID AP et STA viennent du namespace Preferences "nidmi").
    WiFi.persistent(false);

    // Détecter et afficher le MCU
    PinMapper::detectMcu();
    PinMapper::printMappings();

    // Nettoyer les anciens réglages NVS si nécessaire
    // (décommentez la ligne suivante pour forcer le reset)
    // Preferences::clear("nidmi\n\n");

    // Lire nom serveur + STA depuis NVS (ne pas appeler get* si begin a échoué)
    Preferences preferences;
    String serverName = "nidmi";
    g_staSsid = "";
    g_staPass = "";
    g_staIpStr = "";
    g_staGwStr = "";
    g_staSnStr = "";
    bool touchEnabled = false;
    const bool usbMidiEnabled = nidmi_usb_midi_enabled_at_compile_time();

    if (preferences.begin("nidmi", true)) {
        serverName = preferences.getString("mdns_name", "nidmi");
        g_staSsid = preferences.getString("sta_ssid", "");
        g_staPass = preferences.getString("sta_pass", "");
        g_staIpStr = preferences.getString("sta_ip", "");
        g_staGwStr = preferences.getString("sta_gw", "");
        g_staSnStr = preferences.getString("sta_sn", "");
        touchEnabled = preferences.getBool("touch_enabled", false);
        preferences.end();
    } else {
        Serial.println("[NiDMI] ERREUR: ouverture NVS en lecture échouée - NVS peut être corrompue");
        Serial.println("[NiDMI] Utilisation des valeurs par défaut. Flashez nidmi_clear_nvs pour réinitialiser.");
    }
    
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

    // Démarre l’AP : AP seul si aucun STA en NVS (évite soucis d’association client en APSTA « vide »)
    Serial.printf("[MEM] avant WiFi: %d\n", (int)ESP.getFreeHeap());
    serverCore.begin(apSsid, apPass, host, g_staSsid.length() == 0);
    Serial.printf("[MEM] apres WiFi+serveur: %d\n", (int)ESP.getFreeHeap());

    touchDiag("APRES WiFi/serveur");

    // Tente STA après que le mode APSTA soit configuré
    if (g_staSsid.length() > 0) {
        if (g_staIpStr.length() > 0 && g_staGwStr.length() > 0 && g_staSnStr.length() > 0) {
            IPAddress ip, gw, sn;
            if (ip.fromString(g_staIpStr) && gw.fromString(g_staGwStr) && sn.fromString(g_staSnStr)) {
                serverCore.setStaticStaIp(ip, gw, sn);
                Serial.printf("[NiDMI] STA static IP: %s GW: %s SN: %s\n", g_staIpStr.c_str(), g_staGwStr.c_str(), g_staSnStr.c_str());
            }
        }
        serverCore.connectSta(g_staSsid.c_str(), g_staPass.length() > 0 ? g_staPass.c_str() : nullptr);
    } else {
        Serial.println("[NiDMI] No STA configuration found");
    }
    
    // USB-MIDI : NIDMI_USB_MIDI_ENABLED_AT_COMPILE_TIME dans UsbMidiManager.h (pas NVS)
    g_midiRouter.enableUsbMidi(usbMidiEnabled);

    // Initialiser MidiRouter (qui initialisera USB MIDI si activé et supporté)
    g_midiRouter.begin();
    Serial.printf("[MEM] apres MidiRouter: %d\n", (int)ESP.getFreeHeap());
    
    // Initialiser RTP-MIDI
    serverCore.rtpMidi().begin(serverName.c_str());
    Serial.printf("[MEM] apres RTP-MIDI: %d\n", (int)ESP.getFreeHeap());
    
    // Initialiser Bluetooth MIDI
    serverCore.bluetooth().begin(serverName.c_str());
    Serial.printf("[MEM] apres Bluetooth: %d\n", (int)ESP.getFreeHeap());
    
    touchDiag("AVANT ComponentManager.begin");

    // Initialiser ComponentManager
    g_componentManager.begin(&g_midiRouter);
    Serial.printf("[MEM] apres ComponentManager: %d\n", (int)ESP.getFreeHeap());

    touchDiag("APRES ComponentManager.begin (MuxTask+MidiTask demarres)");
    
    Serial.println("[NiDMI] Ready");
    NIDMI_WEB_LOG("[NiDMI] Ready (console web dispo sur S3 si activée)");
    Serial.print("  AP SSID: "); Serial.println(apSsid);
    Serial.print("  AP PASS: "); Serial.println(apPass);
    Serial.print("  AP IP: "); Serial.println(WiFi.softAPIP());
    Serial.print("  mDNS: http://"); Serial.print(host); Serial.println(".local/");
    Serial.print("  RTP-MIDI: "); Serial.println(serverCore.rtpMidi().isInitialized() ? "Initialized" : "Failed");
    Serial.print("  Bluetooth: "); Serial.println(serverCore.bluetooth().isInitialized() ? "Initialized" : "Failed");
    Serial.printf("Touch Enabled: %s\n", touchEnabled ? "true" : "false");
    Serial.println();
}

void nidmi_loop() {
    // Redémarrage différé (laisse le temps à la réponse HTTP et à la NVS de se fermer proprement)
    if (g_requestReboot && (millis() - g_rebootRequestTime >= 2000)) {
        ESP.restart();
    }

    // Tentative de reconnexion STA automatique si des identifiants sont connus
    if (g_staSsid.length() > 0) {
        wl_status_t staStatus = WiFi.status();
        unsigned long now = millis();
        if (staStatus != WL_CONNECTED &&
            now - g_lastStaConnectAttempt >= STA_RECONNECT_INTERVAL_MS) {
            Serial.println("[NiDMI] STA not connected, attempting automatic reconnection...");
            // Reconfigurer éventuellement l'IP statique
            if (g_staIpStr.length() > 0 && g_staGwStr.length() > 0 && g_staSnStr.length() > 0) {
                IPAddress ip, gw, sn;
                if (ip.fromString(g_staIpStr) && gw.fromString(g_staGwStr) && sn.fromString(g_staSnStr)) {
                    serverCore.setStaticStaIp(ip, gw, sn);
                }
            }
            serverCore.connectSta(g_staSsid.c_str(), g_staPass.length() > 0 ? g_staPass.c_str() : nullptr);
            g_lastStaConnectAttempt = now;
        }
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
