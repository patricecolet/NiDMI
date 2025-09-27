#include "ServerCore.h"
#include <ESPmDNS.h>
#include <Preferences.h>

// Déclaration de la fonction setupHttp définie dans WebAPI.cpp
void setupWebAPI(AsyncWebServer& server, AsyncWebSocket& ws);

// Instance globale
ServerCore serverCore;

ServerCore::ServerCore()
    : server(80), ws("/ws") {}

void ServerCore::begin(const char* apSsid, const char* apPass, const char* hostname) {
    WiFi.mode(WIFI_MODE_APSTA);
    WiFi.softAP(apSsid, apPass);
    IPAddress apIp = WiFi.softAPIP();
    
    Serial.begin(115200);
    Serial.println();
    Serial.println("[ServerCore] AP up");
    Serial.print("  SSID: "); Serial.println(apSsid);
    Serial.print("  PASS: "); Serial.println(apPass);
    Serial.print("  AP IP: "); Serial.println(apIp);

    // Configuration mDNS - Détecter le mode automatiquement
    bool staConnected = (WiFi.status() == WL_CONNECTED);
    Serial.printf("[ServerCore] Starting mDNS for %s mode...\n", staConnected ? "STA" : "AP");
    if (staConnected) {
        Serial.printf("[ServerCore] STA IP: %s\n", WiFi.localIP().toString().c_str());
    }
    Serial.print("[ServerCore] Starting mDNS with hostname: "); Serial.println(hostname);
    
    // Essayer plusieurs noms mDNS
    String mdnsNames[] = {hostname, "esp32server", "esp32", "esp32rtpmidi"};
    bool mdnsOk = false;
    String workingName = "";
    
    for (int i = 0; i < 4; i++) {
        Serial.print("[ServerCore] Trying mDNS name: "); Serial.println(mdnsNames[i]);
        if (MDNS.begin(mdnsNames[i].c_str())) {
            MDNS.addService("http", "tcp", 80);
            mdnsOk = true;
            workingName = mdnsNames[i];
            Serial.print("[ServerCore] mDNS success: http://"); Serial.print(workingName); Serial.println(".local/");
            break;
        } else {
            Serial.print("[ServerCore] mDNS failed for: "); Serial.println(mdnsNames[i]);
        }
        delay(500);
    }
    
    if (mdnsOk) {
        Serial.println("[ServerCore] HTTP service registered");
        
        // mDNS configuré avec succès
        Serial.println("[ServerCore] mDNS configuration complete");
        
        // Debug mDNS
        Serial.println("[ServerCore] mDNS Debug Info:");
        Serial.printf("  Hostname: %s\n", workingName.c_str());
        Serial.printf("  STA Connected: %s\n", staConnected ? "Yes" : "No");
        if (staConnected) {
            Serial.printf("  STA IP: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("  STA Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
            Serial.printf("  STA Subnet: %s\n", WiFi.subnetMask().toString().c_str());
        }
        Serial.printf("  AP IP: %s\n", WiFi.softAPIP().toString().c_str());
        Serial.printf("  WiFi Mode: %d\n", WiFi.getMode());
        
        if (staConnected) {
            Serial.printf("[ServerCore] Access via: http://%s.local/ or http://%s/\n", workingName.c_str(), WiFi.localIP().toString().c_str());
        } else {
            Serial.printf("[ServerCore] Access via: http://%s.local/ or http://%s/\n", workingName.c_str(), WiFi.softAPIP().toString().c_str());
        }
        Serial.println("[ServerCore] Note: mDNS resolution may take a few seconds to propagate");
        Serial.println("[ServerCore] If .local doesn't work, use direct IP address");
    } else {
        Serial.println("[ServerCore] All mDNS attempts failed - using direct IP only");
        if (staConnected) {
            Serial.printf("[ServerCore] Use direct IP: http://%s/\n", WiFi.localIP().toString().c_str());
        } else {
            Serial.printf("[ServerCore] Use direct IP: http://%s/\n", WiFi.softAPIP().toString().c_str());
        }
    }
    
    // Configuration des endpoints HTTP
    Serial.println("[ServerCore] Setting up WebAPI...");
    setupWebAPI(server, ws);
    Serial.println("[ServerCore] Starting HTTP server...");
    server.begin();
    Serial.println("[ServerCore] HTTP server started on / (Async)");
    Serial.println("[ServerCore] ServerCore initialization complete!");
}

void ServerCore::connectSta(const char* staSsid, const char* staPass) {
    Serial.printf("[ServerCore] Connecting to STA: %s\n", staSsid);
    
    if (useStaticSta) {
        Serial.printf("[ServerCore] Using static IP: %s\n", staIp.toString().c_str());
        WiFi.config(staIp, staGw, staSn);
    }
    
    WiFi.begin(staSsid, staPass);
    
    // Attendre la connexion avec timeout
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[ServerCore] STA connected! IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.printf("\n[ServerCore] STA connection failed after %d attempts\n", attempts);
        Serial.printf("[ServerCore] WiFi status: %d\n", WiFi.status());
    }
}

void ServerCore::setStaticStaIp(IPAddress ip, IPAddress gateway, IPAddress subnet) {
    useStaticSta = true; 
    staIp = ip; 
    staGw = gateway; 
    staSn = subnet;
}

void ServerCore::update() {
    // Mise à jour du WebSocket
    ws.cleanupClients();
    
    // Mise à jour RTP-MIDI
    rtpMidiInstance.update();
}

void ServerCore::reconfigureMdns(const char* hostname) {
    Serial.println("[ServerCore] Reconfiguring mDNS for STA mode...");
    
    // Arrêter mDNS existant
    MDNS.end();
    delay(1000);
    
    // Vérifier si STA est connecté
    bool staConnected = (WiFi.status() == WL_CONNECTED);
    if (!staConnected) {
        Serial.println("[ServerCore] STA not connected, skipping mDNS reconfiguration");
        return;
    }
    
    Serial.printf("[ServerCore] STA IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.println("[ServerCore] Starting mDNS for STA mode...");
    Serial.print("[ServerCore] Starting mDNS with hostname: "); Serial.println(hostname);
    
    // Essayer plusieurs noms mDNS
    String mdnsNames[] = {hostname, "esp32server", "esp32", "esp32rtpmidi"};
    bool mdnsOk = false;
    String workingName = "";
    
    for (int i = 0; i < 4; i++) {
        Serial.print("[ServerCore] Trying mDNS name: "); Serial.println(mdnsNames[i]);
        if (MDNS.begin(mdnsNames[i].c_str())) {
            MDNS.addService("http", "tcp", 80);
            mdnsOk = true;
            workingName = mdnsNames[i];
            Serial.print("[ServerCore] mDNS success: http://"); Serial.print(workingName); Serial.println(".local/");
            break;
        } else {
            Serial.print("[ServerCore] mDNS failed for: "); Serial.println(mdnsNames[i]);
        }
        delay(500);
    }
    
    if (mdnsOk) {
        Serial.println("[ServerCore] HTTP service registered");
        Serial.printf("[ServerCore] mDNS service: %s.local:80\n", workingName.c_str());
        
        // mDNS configuré avec succès
        Serial.println("[ServerCore] mDNS configuration complete");
        if (staConnected) {
            Serial.printf("[ServerCore] Access via: http://%s.local/ or http://%s/\n", workingName.c_str(), WiFi.localIP().toString().c_str());
        } else {
            Serial.printf("[ServerCore] Access via: http://%s.local/ or http://%s/\n", workingName.c_str(), WiFi.softAPIP().toString().c_str());
        }
        Serial.println("[ServerCore] Note: mDNS resolution may take a few seconds to propagate");
    } else {
        Serial.println("[ServerCore] All mDNS attempts failed - using direct IP only");
        if (staConnected) {
            Serial.printf("[ServerCore] Use direct IP: http://%s/\n", WiFi.localIP().toString().c_str());
        } else {
            Serial.printf("[ServerCore] Use direct IP: http://%s/\n", WiFi.softAPIP().toString().c_str());
        }
    }
}

AsyncWebServer& ServerCore::web() { 
    return server; 
}

AsyncWebSocket& ServerCore::websocket() { 
    return ws; 
}

RtpMidi& ServerCore::rtpMidi() { 
    return rtpMidiInstance; 
}
