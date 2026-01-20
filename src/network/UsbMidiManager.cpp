#include "UsbMidiManager.h"

UsbMidiManager::UsbMidiManager() 
#ifdef NIDMI_USB_MIDI_SUPPORTED
    : usbMidi(nullptr), usbInitialized(false), isStarted(false), available(false) {
#else
    : isStarted(false), available(false) {
#endif
}

UsbMidiManager::~UsbMidiManager() {
    stop();
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
    // Vérifier que USB-OTG est activé
    if (!isUsbOtgEnabled()) {
        Serial.println("[USB-MIDI] ERREUR: USB-OTG non activé!");
        Serial.println("[USB-MIDI] Vérifiez que le fichier ci.json contient CONFIG_SOC_USB_OTG_SUPPORTED=y");
        Serial.println("[USB-MIDI] Ou dans Arduino IDE: Outils > USB Type > USB-OTG (TinyUSB)");
        available = false;
        return false;
    }
    
    // Initialiser USB MIDI
    usbMidi = new USBMIDI();
    usbMidi->begin();
    USB.begin();
    
    usbInitialized = true;
    isStarted = true;
    available = true;
    
    Serial.println("[USB-MIDI] Initialisé (ESP32-S3 avec USB-OTG)");
    return true;
#else
    Serial.println("[USB-MIDI] Non supporté sur ce MCU (ESP32-S3 requis)");
    available = false;
    return false;
#endif
}

void UsbMidiManager::stop() {
#ifdef NIDMI_USB_MIDI_SUPPORTED
    if (isStarted && usbInitialized) {
        if (usbMidi) {
            delete usbMidi;
            usbMidi = nullptr;
        }
        usbInitialized = false;
    }
#endif
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
