#include "MidiRouter.h"
#include <Arduino.h>

// Dépendances vers le serveur existant
#include "../Esp32Server.h"

extern Esp32Server esp32Server;

MidiRouter::MidiRouter()
    : rtpEnabled(true), oscEnabled(true), oscToSta(true), oscPort(8000), defaultChannel(1) {}

MidiRouter::~MidiRouter() {}

void MidiRouter::begin() {
    // Rien ici: on s'appuie sur esp32Server pour RTP/OSC
}

void MidiRouter::update() {
    // Mise à jour RTP si nécessaire
    esp32Server.rtpMidi().update();
}

void MidiRouter::sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    const uint8_t ch = channel ? channel : defaultChannel;
    if (rtpEnabled) {
        esp32Server.rtpMidi().sendNoteOn(ch, note, velocity);
    }
    // Optionnel: route OSC si disponible côté serveur
    // Activez avec -DESP32SERVER_ENABLE_OSC_ROUTER et implémentez les wrappers dans Esp32Server
    #ifdef ESP32SERVER_ENABLE_OSC_ROUTER
    if (oscEnabled) {
        esp32Server.sendOscNote(ch, note, velocity, oscToSta, oscPort);
    }
    #endif
}

void MidiRouter::sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
    const uint8_t ch = channel ? channel : defaultChannel;
    if (rtpEnabled) {
        esp32Server.rtpMidi().sendNoteOff(ch, note, velocity);
    }
    #ifdef ESP32SERVER_ENABLE_OSC_ROUTER
    if (oscEnabled) {
        esp32Server.sendOscNoteOff(ch, note, velocity, oscToSta, oscPort);
    }
    #endif
}

void MidiRouter::sendControlChange(uint8_t channel, uint8_t control, uint8_t value) {
    const uint8_t ch = channel ? channel : defaultChannel;
    if (rtpEnabled) {
        esp32Server.rtpMidi().sendControlChange(ch, control, value);
    }
    #ifdef ESP32SERVER_ENABLE_OSC_ROUTER
    if (oscEnabled) {
        esp32Server.sendOscCC(ch, control, value, oscToSta, oscPort);
    }
    #endif
}

void MidiRouter::enableRtpMidi(bool enabled) { rtpEnabled = enabled; }
void MidiRouter::enableOsc(bool enabled) { oscEnabled = enabled; }
void MidiRouter::setOscTargetSta(bool sta) { oscToSta = sta; }
void MidiRouter::setOscPort(uint16_t port) { oscPort = port; }
void MidiRouter::setMidiChannel(uint8_t channel) { defaultChannel = channel; }


