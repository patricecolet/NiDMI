#ifndef RTP_MIDI_H
#define RTP_MIDI_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiUDP.h>
#include <AppleMIDI.h>

USING_NAMESPACE_APPLEMIDI

class RtpMidi {
private:
    String deviceName;
    bool isStarted;
    
public:
    RtpMidi();
    ~RtpMidi();
    
    bool begin(const String& name);
    void stop();
    void update();
    
    void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
    void sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity);
    void sendControlChange(uint8_t channel, uint8_t control, uint8_t value);
    
    // Nouveaux messages MIDI
    void sendProgramChange(uint8_t channel, uint8_t program);
    void sendPitchBend(uint8_t channel, int bend);
    void sendAftertouch(uint8_t channel, uint8_t pressure);
    void sendClock();
    void sendStart();
    void sendStop();
    void sendContinue();
    
    bool isConnected() const;
    bool isInitialized() const { return isStarted; }
    String getName() const { return deviceName; }
};

#endif