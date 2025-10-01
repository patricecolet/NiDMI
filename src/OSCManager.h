#ifndef OSCMANAGER_H
#define OSCMANAGER_H

#include <Arduino.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>

// Interfaces réseau pour l'envoi OSC
enum OSCInterface : uint8_t {
    OSC_INTERFACE_AP = 0,
    OSC_INTERFACE_STA = 1,
    OSC_INTERFACE_BOTH = 2
};

typedef void (*OSCMessageCallback)(const String& address, float value);

class OSCManager {
public:
    OSCManager();
    ~OSCManager();

    bool begin(const String& target_ip, uint16_t target_port, uint16_t local_port);
    void end();

    void setEnabled(bool enable);
    bool isEnabled() const;
    bool isInitialized() const;

    // Envoi de messages
    bool sendFloat(const String& address, float value);
    bool sendInt(const String& address, int value);
    bool sendNote(const String& address, uint8_t note, uint8_t velocity);
    bool sendMidiMessage(const String& address, uint8_t data1, uint8_t data2, uint8_t channel);
    bool sendMultiFloat(const String& address, float* values, int count);

    // Cible unicast
    void setTarget(const String& target_ip, uint16_t target_port);
    String getTargetIP() const;
    uint16_t getTargetPort() const;

    // Broadcast
    void setBroadcast(bool enable);
    bool isBroadcastEnabled() const;
    void setInterface(uint8_t interface);

    // Réception
    void setMessageCallback(OSCMessageCallback callback);
    void update();
    void printStatus() const;

private:
    bool sendOSCMessage(OSCMessage& msg);

private:
    WiFiUDP udp;
    String targetIP;
    uint16_t targetPort;
    uint16_t localPort;
    bool initialized;
    bool enabled;

    // Broadcast
    bool broadcastEnabled;
    String broadcastIP; // calculée au démarrage selon AP/STA
    uint8_t networkInterface; // OSCInterface
    OSCMessageCallback messageCallback;
};

#endif // OSCMANAGER_H


