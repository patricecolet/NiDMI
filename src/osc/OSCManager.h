#pragma once

#include <Arduino.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <functional>

/* Les liens de diffusion sont un masque (osc_links::AP | STA | USB), pas un
   choix exclusif : voir OSCLinks.h. */
#include "OSCLinks.h"

typedef std::function<void(const String&, float, const String&)> OSCMessageCallback;

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
    uint8_t getInterface() const;

    // Réception
    void setMessageCallback(OSCMessageCallback callback);
    void update();
    void printStatus() const;
    void disconnect();

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