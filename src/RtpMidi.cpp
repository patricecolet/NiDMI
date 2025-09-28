#include "RtpMidi.h"
#include <ESPmDNS.h>
#include <Preferences.h>
#include "ComponentManager.h"

USING_NAMESPACE_APPLEMIDI

// Créer l'instance AppleMIDI avec un nom par défaut
// Le nom sera changé dynamiquement dans begin()
APPLEMIDI_CREATE_INSTANCE(WiFiUDP, MIDI, "ESP32-MIDI", 5004);

RtpMidi::RtpMidi() : isStarted(false) {
}

RtpMidi::~RtpMidi() {
    stop();
}

bool RtpMidi::begin(const String& name) {
    // Lire le nom depuis les préférences (priorité) ou utiliser le paramètre
    Preferences preferences;
    preferences.begin("esp32server", false);
    String storedName = preferences.getString("rtp_name", "");
    preferences.end();
    
    // Utiliser le nom des préférences s'il existe, sinon le paramètre
    if(storedName.length() > 0) {
        deviceName = storedName;
        Serial.println("RTP-MIDI: Nom depuis les préférences: " + deviceName);
    } else {
        deviceName = name;
        Serial.println("RTP-MIDI: Nom depuis le paramètre: " + deviceName);
    }
    
    // Démarrer AppleMIDI
    AppleMIDI.begin();
    Serial.println("RTP-MIDI: AppleMIDI démarré");
    
    // Changer le nom affiché dans AppleMIDI (pour Audio MIDI Setup)
    AppleMIDI.setName(deviceName.c_str());
    Serial.println("RTP-MIDI: Nom AppleMIDI défini: " + deviceName);
    
    // Lire le nom mDNS depuis les préférences (configuré par le serveur)
    preferences.begin("esp32server", false);
    String mdnsName = preferences.getString("mdns_name", "");
    preferences.end();
    
    Serial.println("RTP-MIDI: Nom mDNS serveur: '" + mdnsName + "'");
    
    // Ajouter le service RTP-MIDI au mDNS existant
    // On assume que mDNS est déjà initialisé par le serveur
    MDNS.addService("apple-midi", "udp", AppleMIDI.getPort());
    Serial.println("RTP-MIDI: Service mDNS ajouté sur le port " + String(AppleMIDI.getPort()));
    Serial.println("RTP-MIDI: Service accessible via " + mdnsName + ".local");
    
    // Afficher l'IP pour debug
    Serial.print("RTP-MIDI: IP de l'ESP32: ");
    if(WiFi.status() == WL_CONNECTED) {
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("Non connecté en STA");
    }
    
    // Configurer les callbacks de connexion
    AppleMIDI.setHandleConnected([](const APPLEMIDI_NAMESPACE::ssrc_t & ssrc, const char* name) {
        Serial.println("RTP-MIDI: Connexion reçue de " + String(name));
    });
    
    AppleMIDI.setHandleDisconnected([](const APPLEMIDI_NAMESPACE::ssrc_t & ssrc) {
        Serial.println("RTP-MIDI: Déconnexion reçue");
    });
    
    // Configurer les callbacks MIDI standard pour éviter l'écho
    MIDI.setHandleNoteOn([](byte channel, byte note, byte velocity) {
        Serial.printf("Note On: ch%d note%d vel%d\n", channel, note, velocity);
        extern ComponentManager g_componentManager;
        g_componentManager.handleMidiNoteOn(channel, note, velocity);
    });

    MIDI.setHandleNoteOff([](byte channel, byte note, byte velocity) {
        Serial.printf("Note Off: ch%d note%d vel%d\n", channel, note, velocity);
        extern ComponentManager g_componentManager;
        g_componentManager.handleMidiNoteOff(channel, note, velocity);
    });

    MIDI.setHandleControlChange([](byte channel, byte control, byte value) {
        Serial.printf("CC: ch%d cc%d val%d\n", channel, control, value);
        extern ComponentManager g_componentManager;
        g_componentManager.handleMidiControlChange(channel, control, value);
    });
    
    isStarted = true;
    Serial.println("RTP-MIDI: Prêt à recevoir des connexions");
    Serial.println("RTP-MIDI: Vérifiez que votre Mac est sur le même réseau Wi-Fi");
    Serial.println("RTP-MIDI: Ouvrez Audio MIDI Setup > Window > Show MIDI Studio");
    Serial.println("RTP-MIDI: L'ESP32 apparaîtra avec le nom: " + deviceName);
    return true;
}

void RtpMidi::stop() {
    if (isStarted) {
        AppleMIDI.end();
        isStarted = false;
        Serial.println("RTP-MIDI: Arrêté");
    }
}

void RtpMidi::update() {
    if (!isStarted) return;
    
    // Nécessaire pour que les callbacks MIDI soient appelés
    // Les callbacks évitent l'écho en traitant directement les messages
    MIDI.read();
}

void RtpMidi::sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (!isStarted) return;
    MIDI.sendNoteOn(note, velocity, channel);
}

void RtpMidi::sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (!isStarted) return;
    MIDI.sendNoteOff(note, velocity, channel);
}

void RtpMidi::sendControlChange(uint8_t channel, uint8_t control, uint8_t value) {
    if (!isStarted) return;
    MIDI.sendControlChange(control, value, channel);
}

bool RtpMidi::isConnected() const {
    // Pour l'instant, on considère qu'on est connecté si RTP-MIDI est démarré
    // Dans un vrai projet, il faudrait implémenter un système de comptage des connexions
    return isStarted;
}