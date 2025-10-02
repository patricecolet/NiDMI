#ifndef SERVERCORE_H
#define SERVERCORE_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <IPAddress.h>
#include "RtpMidi.h"
#include "BluetoothManager.h"

/**
 * @brief Infrastructure serveur (WiFi, mDNS, RTP-MIDI, Web)
 * 
 * Cette classe gère l'infrastructure de base :
 * - Access Point Wi-Fi
 * - Station Wi-Fi (optionnelle)
 * - mDNS
 * - Serveur HTTP asynchrone
 * - WebSocket
 * - RTP-MIDI
 * - Bluetooth MIDI
 */
class ServerCore {
private:
    AsyncWebServer server;
    AsyncWebSocket ws;
    RtpMidi rtpMidiInstance;
    BluetoothManager bluetoothInstance;
    bool useStaticSta = false;
    IPAddress staIp, staGw, staSn;
    
public:
    ServerCore();
    
    // Initialisation
    void begin(const char* apSsid, const char* apPass, const char* hostname);
    void connectSta(const char* staSsid, const char* staPass);
    void setStaticStaIp(IPAddress ip, IPAddress gateway, IPAddress subnet);
    void reconfigureMdns(const char* hostname);
    
    // Accès aux services
    AsyncWebServer& web();
    AsyncWebSocket& websocket();
    RtpMidi& rtpMidi();
    BluetoothManager& bluetooth();
    
    // Mise à jour périodique
    void update();
};

// Instance globale du serveur
extern ServerCore serverCore;

#endif // SERVERCORE_H
