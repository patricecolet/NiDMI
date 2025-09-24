// Interface d'envoi MIDI et OSC unifiée
#pragma once

#include <Arduino.h>

class MidiSender {
public:
    virtual ~MidiSender() {}

    virtual void begin() = 0;
    virtual void update() = 0;

    // Messages MIDI basiques
    virtual void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) = 0;
    virtual void sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) = 0;
    virtual void sendControlChange(uint8_t channel, uint8_t control, uint8_t value) = 0;
};


