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

#endif