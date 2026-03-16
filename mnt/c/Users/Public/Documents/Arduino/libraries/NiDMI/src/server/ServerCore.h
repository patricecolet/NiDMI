#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <IPAddress.h>
#include "../network/RtpMidi.h"
#include "../network/BluetoothManager.h"
#include "../network/UsbMidiManager.h"

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
 * - USB MIDI (ESP32-S3 uniquement)
 */
class ServerCore {
private:
    AsyncWebServer server;
    AsyncWebSocket ws;
    RtpMidi rtpMidiInstance;
    BluetoothManager bluetoothInstance;
    UsbMidiManager usbMidiInstance;
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
    UsbMidiManager& usbMidi();
    
    // Mise à jour périodique
    void update();
};

// Note: L'instance globale serverCore est déclarée dans Globals.h
