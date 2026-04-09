#include "UsbMidiManager.h"

#ifdef NIDMI_USB_MIDI_SUPPORTED
#include <Preferences.h>
#include <esp_arduino_version.h>
#include <sdkconfig.h>
#if CONFIG_TINYUSB_CDC_ENABLED
#include <USBCDC.h>
// Instance locale uniquement en mode USB-OTG ET cdc_on_boot=0.
// En HWCDC (ARDUINO_USB_MODE=1) : TinyUSB inactif, USBCDC n'a rien à faire.
// En cdc_on_boot=1             : le framework crée déjà USBSerial/Serial.
#if !ARDUINO_USB_MODE && !ARDUINO_USB_CDC_ON_BOOT
static USBCDC nidmiUsbMidiCdc;
#endif
#endif

// Même logique que nidmi_begin() : nom mDNS / SSID AP stocké en NVS sous "mdns_name"
static String nidmiUsbMidiHostNameFromNvs() {
    Preferences prefs;
    String name = "nidmi";
    if (prefs.begin("nidmi", true)) {
        name = prefs.getString("mdns_name", "nidmi");
        prefs.end();
    }
    name.trim();
    name.replace("\n", "");
    name.replace("\r", "");
    name.replace("\t", "");
    if (name.length() == 0) {
        name = "nidmi";
    }
    // SSID + limite du constructeur USBMIDI(const char*) sur core Arduino-ESP32 >= 3.3.1
    const size_t kMaxUsbMidiName = 32;
    if (name.length() > kMaxUsbMidiName) {
        name = name.substring(0, kMaxUsbMidiName);
    }
    return name;
}
#endif

UsbMidiManager::UsbMidiManager() 
#ifdef NIDMI_USB_MIDI_SUPPORTED
    : usbMidi(nullptr), usbInitialized(false), isStarted(false), available(false) {
#else
    : isStarted(false), available(false) {
#endif
}

UsbMidiManager::~UsbMidiManager() {
    stop();

#ifdef NIDMI_USB_MIDI_SUPPORTED
    // Nettoyer explicitement l'objet alloué (évite fuite mémoire si on re-désactive/active souvent)
    if (usbMidi) {
        delete usbMidi;
        usbMidi = nullptr;
    }
#endif
}

bool UsbMidiManager::isSupported() const {
#ifdef NIDMI_USB_MIDI_SUPPORTED
    return true;
#else
    return false;
#endif
}

bool UsbMidiManager::isUsbOtgEnabled() const {
#ifdef NIDMI_USB_MIDI_SUPPORTED
    // Sur ESP32-S3, si sdkconfig.defaults contient CONFIG_SOC_USB_OTG_SUPPORTED=y,
    // USB-OTG est activé au niveau hardware même si ARDUINO_USB_MODE indique mode série.
    // On ne peut pas vérifier sdkconfig.defaults depuis le code C++, donc on fait confiance
    // au fait que si le build a réussi avec sdkconfig.defaults, USB-OTG est disponible.
    
    // Si ARDUINO_USB_MODE est défini et != 1, c'est USB-OTG
    #ifdef ARDUINO_USB_MODE
        #if ARDUINO_USB_MODE != 1
            return true; // USB-OTG activé
        #endif
    #endif
    
    // Si ARDUINO_USB_MODE == 1 ou non défini, on retourne true par défaut
    // car sdkconfig.defaults peut activer USB-OTG indépendamment
    // (l'initialisation USB.begin() échouera si vraiment USB-OTG n'est pas disponible)
    return true;
#else
    return false; // Pas un ESP32-S3
#endif
}

bool UsbMidiManager::begin() {
    if (isStarted) {
        return true;
    }
    
#ifdef NIDMI_USB_MIDI_SUPPORTED
    if (!isUsbOtgEnabled()) {
        Serial.println("[USB-MIDI] ERREUR: USB-OTG non active!");
        available = false;
        return false;
    }

#if ARDUINO_USB_MODE
    // Mode HWCDC (Hardware CDC+JTAG) : TinyUSB n'est pas actif.
    // USBMIDI et USB.begin() ne doivent JAMAIS être appelés ici → crash garanti.
    // La console série est disponible sur le port JTAG (nom long).
    Serial.println("[USB-MIDI] Mode HWCDC: USB-MIDI impossible (TinyUSB inactif).");
    Serial.println("[USB-MIDI] Pour MIDI USB: compiler avec USBMode=USB-OTG dans nidmi.sh.");
    usbInitialized = true;
    isStarted = false;
    available = false;
    return false;
#elif ARDUINO_USB_CDC_ON_BOOT
    // USB-OTG + cdc_on_boot=1 : TinyUSB déjà démarré par le framework AVANT setup().
    // Appeler new USBMIDI() ici crashe le stack USB → port disparaît.
    // La console série (USB CDC) est déjà active sur le port court.
    Serial.println("[USB-MIDI] cdc_on_boot=1: USB-MIDI impossible (TinyUSB deja lance).");
    Serial.println("[USB-MIDI] Pour MIDI USB: desactiver CDCOnBoot dans nidmi.sh.");
    usbInitialized = true;
    isStarted = false;
    available = false;
    return false;
#else
    // USB-OTG + cdc_on_boot=0 : on contrôle l'init USB → CDC + MIDI possible.
    const String hostName = nidmiUsbMidiHostNameFromNvs();

    if (!usbMidi) {
#if CONFIG_TINYUSB_CDC_ENABLED
        // Enregistrer CDC avant MIDI (ordre des interfaces TinyUSB).
        nidmiUsbMidiCdc.begin();
#endif
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 3, 1)
        usbMidi = new USBMIDI(hostName.c_str());
#else
        usbMidi = new USBMIDI();
#endif
    }

    if (!usbInitialized) {
        USB.productName(hostName.c_str());
        usbMidi->begin();
        USB.begin();
        usbInitialized = true;
#if CONFIG_TINYUSB_CDC_ENABLED
        Serial.println("[USB-MIDI] CDC + MIDI USB demarre (cdc_on_boot=0).");
#endif
    }

    isStarted = true;
    available = true;
    Serial.printf("[USB-MIDI] Demarre, nom USB/MIDI: %s\n", hostName.c_str());
    return true;
#endif // ARDUINO_USB_MODE / ARDUINO_USB_CDC_ON_BOOT

#else
    available = false;
    return false;
#endif // NIDMI_USB_MIDI_SUPPORTED
}

bool UsbMidiManager::beginCdc() {
#ifdef NIDMI_USB_MIDI_SUPPORTED
    if (usbInitialized) {
        return true;
    }
#if ARDUINO_USB_MODE
    // Mode HWCDC : Serial est le port JTAG matériel. Rien à initialiser.
    usbInitialized = true;
    return true;
#elif ARDUINO_USB_CDC_ON_BOOT
    // USB-OTG + cdc_on_boot=1 : Serial (USB CDC) déjà actif par le framework. Rien à faire.
    usbInitialized = true;
    return true;
#else
    // USB-OTG + cdc_on_boot=0 : démarrer CDC manuellement (sans MIDI).
    if (!isUsbOtgEnabled()) {
        return false;
    }
    const String hostName = nidmiUsbMidiHostNameFromNvs();
#if CONFIG_TINYUSB_CDC_ENABLED
    nidmiUsbMidiCdc.begin();
#endif
    USB.productName(hostName.c_str());
    USB.begin();
    usbInitialized = true;
    Serial.println("[USB-CDC] CDC USB demarre (cdc_on_boot=0, MIDI desactive).");
    return true;
#endif
#else
    return false;
#endif
}

void UsbMidiManager::stop() {
#ifdef NIDMI_USB_MIDI_SUPPORTED
    // Désactiver le routage (etat "connected/enabled" pour l'API/UI),
    // sans détruire l'instance USB pour limiter les risques de crash
    // lors d'un toggle depuis l'interface web.
    isStarted = false;
    available = false;
#endif
    // Pour les builds non supportés aussi, garantir un état "désactivé"
    isStarted = false;
    available = false;
}

void UsbMidiManager::update() {
    // Vérification de connexion USB si nécessaire
    // Sur ESP32-S3, USB est toujours disponible une fois initialisé
#ifdef NIDMI_USB_MIDI_SUPPORTED
    if (isStarted && usbInitialized) {
        // Lire les messages MIDI entrants si nécessaire
        // (peut être ajouté plus tard pour la réception)
    }
#endif
}

bool UsbMidiManager::isConnected() const {
#ifdef NIDMI_USB_MIDI_SUPPORTED
    return isStarted && usbInitialized && available;
#else
    return false;
#endif
}

void UsbMidiManager::sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
#ifdef NIDMI_USB_MIDI_SUPPORTED
    if (usbMidi && isConnected()) {
        // USB MIDI format: CIN=0x09 pour Note On
        // Status: 0x90-0x9F (Note On, 0x9n où n=channel 0-15)
        uint8_t status = 0x90 | (channel & 0x0F); // Channel 1-16 -> 0-15
        midiEventPacket_t packet = {0x09, status, note, velocity};
        usbMidi->writePacket(&packet);
    }
#endif
}

void UsbMidiManager::sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
#ifdef NIDMI_USB_MIDI_SUPPORTED
    if (usbMidi && isConnected()) {
        // USB MIDI format: CIN=0x08 pour Note Off
        // Status: 0x80-0x8F (Note Off, 0x8n où n=channel 0-15)
        uint8_t status = 0x80 | (channel & 0x0F); // Channel 1-16 -> 0-15
        midiEventPacket_t packet = {0x08, status, note, velocity};
        usbMidi->writePacket(&packet);
    }
#endif
}

void UsbMidiManager::sendControlChange(uint8_t channel, uint8_t control, uint8_t value) {
#ifdef NIDMI_USB_MIDI_SUPPORTED
    if (usbMidi && isConnected()) {
        // USB MIDI format: CIN=0x0B pour Control Change
        // Status: 0xB0-0xBF (Control Change, 0xBn où n=channel 0-15)
        uint8_t status = 0xB0 | (channel & 0x0F); // Channel 1-16 -> 0-15
        midiEventPacket_t packet = {0x0B, status, control, value};
        usbMidi->writePacket(&packet);
    }
#endif
}

void UsbMidiManager::sendProgramChange(uint8_t channel, uint8_t program) {
#ifdef NIDMI_USB_MIDI_SUPPORTED
    if (usbMidi && isConnected()) {
        // USB MIDI format: CIN=0x0C pour Program Change
        // Status: 0xC0-0xCF (Program Change, 0xCn où n=channel 0-15)
        // Data1: program (0-127), Data2: 0x00 (non utilisé)
        uint8_t status = 0xC0 | (channel & 0x0F); // Channel 1-16 -> 0-15
        midiEventPacket_t packet = {0x0C, status, program, 0x00};
        usbMidi->writePacket(&packet);
    }
#endif
}

void UsbMidiManager::sendPitchBend(uint8_t channel, int bend) {
#ifdef NIDMI_USB_MIDI_SUPPORTED
    if (usbMidi && isConnected()) {
        // Convertir bend (-8192 à 8191) en format MIDI (0-16383)
        uint16_t midiBend = (uint16_t)(bend + 8192);
        // USB MIDI format: CIN=0x05 pour Channel Voice Messages à 3 octets
        // Status: 0xE0-0xEF (Pitch Bend Change, 0xEn où n=channel 0-15)
        // Data1: LSB (bits 0-6), Data2: MSB (bits 7-13)
        uint8_t status = 0xE0 | (channel & 0x0F); // Channel 1-16 -> 0-15
        uint8_t lsb = midiBend & 0x7F; // Bits 0-6
        uint8_t msb = (midiBend >> 7) & 0x7F; // Bits 7-13
        midiEventPacket_t packet = {0x05, status, lsb, msb};
        usbMidi->writePacket(&packet);
    }
#endif
}

void UsbMidiManager::sendAftertouch(uint8_t channel, uint8_t pressure) {
#ifdef NIDMI_USB_MIDI_SUPPORTED
    if (usbMidi && isConnected()) {
        // USB MIDI format: CIN=0x04 pour Channel Voice Messages à 2 octets
        // Status: 0xD0-0xDF (Channel Pressure, 0xDn où n=channel 0-15)
        // Data1: pressure (0-127), Data2: 0x00 (non utilisé)
        uint8_t status = 0xD0 | (channel & 0x0F); // Channel 1-16 -> 0-15
        midiEventPacket_t packet = {0x04, status, pressure, 0x00};
        usbMidi->writePacket(&packet);
    }
#endif
}

void UsbMidiManager::sendKeyPressure(uint8_t channel, uint8_t note, uint8_t pressure) {
    // TEMPORAIREMENT DÉSACTIVÉ POUR DEBUG - CRASH SUSPECTÉ
    return;
    /*
#ifdef NIDMI_USB_MIDI_SUPPORTED
    if (usbMidi && isConnected()) {
        // USB MIDI format: CIN=0x04 pour Channel Voice Messages à 2 octets
        // Status: 0xA0-0xAF (Polyphonic Key Pressure, 0xAn où n=channel 0-15)
        // Data1: note (0-127), Data2: pressure (0-127)
        uint8_t status = 0xA0 | (channel & 0x0F); // Channel 1-16 -> 0-15
        midiEventPacket_t packet = {0x04, status, note & 0x7F, pressure & 0x7F};
        usbMidi->writePacket(&packet);
    }
#endif
    */
}

void UsbMidiManager::sendClock() {
#ifdef NIDMI_USB_MIDI_SUPPORTED
    if (usbMidi && isConnected()) {
        // USB MIDI format: header (CIN=0x0F pour Real-Time), byte1=message, byte2=0, byte3=0
        midiEventPacket_t packet = {0x0F, 0xF8, 0x00, 0x00}; // MIDI Clock (0xF8)
        usbMidi->writePacket(&packet);
    }
#endif
}

void UsbMidiManager::sendStart() {
#ifdef NIDMI_USB_MIDI_SUPPORTED
    if (usbMidi && isConnected()) {
        midiEventPacket_t packet = {0x0F, 0xFA, 0x00, 0x00}; // MIDI Start (0xFA)
        usbMidi->writePacket(&packet);
    }
#endif
}

void UsbMidiManager::sendStop() {
#ifdef NIDMI_USB_MIDI_SUPPORTED
    if (usbMidi && isConnected()) {
        midiEventPacket_t packet = {0x0F, 0xFC, 0x00, 0x00}; // MIDI Stop (0xFC)
        usbMidi->writePacket(&packet);
    }
#endif
}

void UsbMidiManager::sendContinue() {
#ifdef NIDMI_USB_MIDI_SUPPORTED
    if (usbMidi && isConnected()) {
        midiEventPacket_t packet = {0x0F, 0xFB, 0x00, 0x00}; // MIDI Continue (0xFB)
        usbMidi->writePacket(&packet);
    }
#endif
}
