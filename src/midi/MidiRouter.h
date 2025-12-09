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
    
    // Nouveaux messages MIDI
    void sendProgramChange(uint8_t channel, uint8_t program) override;
    void sendPitchBend(uint8_t channel, int bend) override;
    void sendAftertouch(uint8_t channel, uint8_t pressure) override;
    void sendClock() override;
    void sendStart() override;
    void sendStop() override;
    void sendContinue() override;

    void enableRtpMidi(bool enabled);
    void enableOsc(bool enabled);
    void enableBluetooth(bool enabled);

    void setOscTargetSta(bool sta);
    void setOscPort(uint16_t port);

    void setMidiChannel(uint8_t channel); // défaut 1
    
    // Réception MIDI pour piloter les LEDs
    void handleMidiNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
    void handleMidiNoteOff(uint8_t channel, uint8_t note, uint8_t velocity);
    void handleMidiControlChange(uint8_t channel, uint8_t control, uint8_t value);

private:
    bool rtpEnabled;
    bool oscEnabled;
    bool bluetoothEnabled;
    bool oscToSta;
    uint16_t oscPort;
    uint8_t defaultChannel;
};


