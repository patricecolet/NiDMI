#ifndef ESP32SERVER_H
#define ESP32SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <IPAddress.h>
#include "RtpMidi.h"

class Esp32Server {
private:
    AsyncWebServer server;
    AsyncWebSocket ws;
    RtpMidi rtpMidiInstance;
    bool useStaticSta = false;
    IPAddress staIp, staGw, staSn;
    
public:
    Esp32Server();
    void begin(const char* apSsid, const char* apPass, const char* hostname);
    void connectSta(const char* staSsid, const char* staPass);
    void setStaticStaIp(IPAddress ip, IPAddress gateway, IPAddress subnet);
    
    AsyncWebServer& web();
    AsyncWebSocket& websocket();
    RtpMidi& rtpMidi();
};

// API style midimap: objet global avec begin()/loop()
struct Esp32ServerAPI {
    void begin();
    void loop();
};

// Instance globale exposée
extern Esp32Server esp32Server;
extern Esp32ServerAPI esp32server;

// Fonction legacy (optionnelle): init unique
void esp32server_begin();

#endif // ESP32SERVER_H