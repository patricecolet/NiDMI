#include "MidiRouter.h"
#include <Arduino.h>

// Dépendances vers le serveur core
#include "../ServerCore.h"
#include "../ComponentManager.h"

extern ServerCore serverCore;

MidiRouter::MidiRouter()
    : rtpEnabled(true), oscEnabled(true), bluetoothEnabled(true), oscToSta(true), oscPort(8000), defaultChannel(1) {}

MidiRouter::~MidiRouter() {}

void MidiRouter::begin() {
    // Rien ici: on s'appuie sur esp32Server pour RTP/OSC
}

void MidiRouter::update() {
    // Mise à jour RTP si nécessaire
    serverCore.rtpMidi().update();
}

void MidiRouter::sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    const uint8_t ch = channel ? channel : defaultChannel;
    if (rtpEnabled) {
        serverCore.rtpMidi().sendNoteOn(ch, note, velocity);
    }
    if (bluetoothEnabled) {
        serverCore.bluetooth().sendNoteOn(ch, note, velocity);
    }
    // Optionnel: route OSC si disponible côté serveur
    // Activez avec -DESP32SERVER_ENABLE_OSC_ROUTER et implémentez les wrappers dans Esp32Server
    #ifdef ESP32SERVER_ENABLE_OSC_ROUTER
    if (oscEnabled) {
        serverCore.sendOscNote(ch, note, velocity, oscToSta, oscPort);
    }
    #endif
}

void MidiRouter::sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
    const uint8_t ch = channel ? channel : defaultChannel;
    if (rtpEnabled) {
        serverCore.rtpMidi().sendNoteOff(ch, note, velocity);
    }
    if (bluetoothEnabled) {
        serverCore.bluetooth().sendNoteOff(ch, note, velocity);
    }
    #ifdef ESP32SERVER_ENABLE_OSC_ROUTER
    if (oscEnabled) {
        serverCore.sendOscNoteOff(ch, note, velocity, oscToSta, oscPort);
    }
    #endif
}

void MidiRouter::sendControlChange(uint8_t channel, uint8_t control, uint8_t value) {
    const uint8_t ch = channel ? channel : defaultChannel;
    if (rtpEnabled) {
        serverCore.rtpMidi().sendControlChange(ch, control, value);
    }
    if (bluetoothEnabled) {
        serverCore.bluetooth().sendControlChange(ch, control, value);
    }
    #ifdef ESP32SERVER_ENABLE_OSC_ROUTER
    if (oscEnabled) {
        serverCore.sendOscCC(ch, control, value, oscToSta, oscPort);
    }
    #endif
}

void MidiRouter::enableRtpMidi(bool enabled) { rtpEnabled = enabled; }
void MidiRouter::enableOsc(bool enabled) { oscEnabled = enabled; }
void MidiRouter::enableBluetooth(bool enabled) { bluetoothEnabled = enabled; }
void MidiRouter::setOscTargetSta(bool sta) { oscToSta = sta; }
void MidiRouter::setOscPort(uint16_t port) { oscPort = port; }
void MidiRouter::setMidiChannel(uint8_t channel) { defaultChannel = channel; }

void MidiRouter::handleMidiNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    // Transmettre au ComponentManager pour piloter les LEDs
    extern ComponentManager g_componentManager;
    g_componentManager.handleMidiNoteOn(channel, note, velocity);
}

void MidiRouter::handleMidiNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
    // Transmettre au ComponentManager pour piloter les LEDs
    extern ComponentManager g_componentManager;
    g_componentManager.handleMidiNoteOff(channel, note, velocity);
}

void MidiRouter::handleMidiControlChange(uint8_t channel, uint8_t control, uint8_t value) {
    // Transmettre au ComponentManager pour piloter les LEDs
    extern ComponentManager g_componentManager;
    g_componentManager.handleMidiControlChange(channel, control, value);
}


