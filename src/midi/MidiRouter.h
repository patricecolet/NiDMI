// Routeur vers implémentations RTP-MIDI et OSC existantes
#pragma once

#include <Arduino.h>
#include "MidiSender.h"

class MidiRouter : public MidiSender {
public:
    MidiRouter();
    ~MidiRouter() override;

    void begin() override;
    void update() override;

    void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) override;
    void sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) override;
    void sendControlChange(uint8_t channel, uint8_t control, uint8_t value) override;

    void enableRtpMidi(bool enabled);
    void enableOsc(bool enabled);

    void setOscTargetSta(bool sta);
    void setOscPort(uint16_t port);

    void setMidiChannel(uint8_t channel); // défaut 1

private:
    bool rtpEnabled;
    bool oscEnabled;
    bool oscToSta;
    uint16_t oscPort;
    uint8_t defaultChannel;
};


