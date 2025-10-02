#ifndef debug_network
#define debug_network(...) ((void)0)
#endif
#include "OSCManager.h"
#include <WiFi.h>

OSCManager::OSCManager() : 
    targetIP(""),
    targetPort(8000),
    localPort(4000),
    initialized(false),
    enabled(false),
    broadcastEnabled(false),
    broadcastIP(""),
    networkInterface(OSC_INTERFACE_AP),
    messageCallback(nullptr) {
}

OSCManager::~OSCManager() {
    end();
}

bool OSCManager::begin(const String& target_ip, uint16_t target_port, uint16_t local_port) {
    if (initialized) {
        end();
    }

    targetIP = target_ip;
    targetPort = target_port;
    localPort = local_port;

    // Démarrer UDP pour l'envoi
    if (!udp.begin(localPort)) {
        debug_network( "[OSC] Erreur: Impossible de démarrer UDP\n");
        return false;
    }

    initialized = true;
    enabled = true;

    // Calculer l'adresse broadcast selon le réseau actuel
    if (WiFi.status() == WL_CONNECTED) {
        // Mode STA : broadcast du réseau WiFi
        IPAddress ip = WiFi.localIP();
        IPAddress subnet = WiFi.subnetMask();
        IPAddress broadcast = IPAddress(ip[0] | (~subnet[0]), 
                                       ip[1] | (~subnet[1]), 
                                       ip[2] | (~subnet[2]), 
                                       ip[3] | (~subnet[3]));
        broadcastIP = broadcast.toString();
    } else {
        // Mode AP : broadcast du réseau AP (192.168.4.255)
        broadcastIP = "192.168.4.255";
    }

    debug_network( "[OSC] Démarré - Local:%d -> %s:%d\n", 
                  localPort, targetIP.c_str(), targetPort);
    debug_network( "[OSC] Broadcast disponible: %s:%d\n", 
                  broadcastIP.c_str(), targetPort);
    debug_network( "[OSC] Interface réseau: %d (0=AP, 1=STA, 2=BOTH)\n", networkInterface);
    debug_network( "[OSC] Broadcast activé: %s\n", broadcastEnabled ? "OUI" : "NON\n");
    
    return true;
}

void OSCManager::end() {
    if (initialized) {
        udp.stop();
        initialized = false;
        enabled = false;
        debug_network( "[OSC] Arrêté\n");
    }
}

void OSCManager::setEnabled(bool enable) {
    enabled = enable && initialized;
    debug_network( "[OSC] %s\n", enabled ? "Activé" : "Désactivé\n");
}

bool OSCManager::isEnabled() const {
    return enabled && initialized;
}

bool OSCManager::sendFloat(const String& address, float value) {
    OSCMessage msg(address.c_str());
    msg.add(value);
    
    #ifdef ESP32SERVER_debug_osc
    debug_network( "[OSC] Préparation %s %.3f\n", address.c_str(), value);
    #endif
    
    return sendOSCMessage(msg);
}

bool OSCManager::sendInt(const String& address, int value) {
    if (!isEnabled()) {
        return false;
    }

    OSCMessage msg(address.c_str());
    msg.add((int32_t)value);

    return sendOSCMessage(msg);
}

bool OSCManager::sendNote(const String& address, uint8_t note, uint8_t velocity) {
    if (!isEnabled()) {
        return false;
    }

    OSCMessage msg(address.c_str());
    msg.add((int32_t)note);
    msg.add((int32_t)velocity);

    return sendOSCMessage(msg);
}

bool OSCManager::sendMidiMessage(const String& address, uint8_t data1, uint8_t data2, uint8_t channel) {
    if (!isEnabled()) {
        return false;
    }

    OSCMessage msg(address.c_str());
    msg.add((int32_t)data1);    // Note/Control
    msg.add((int32_t)data2);    // Velocity/Value  
    msg.add((int32_t)channel);  // Canal MIDI

    return sendOSCMessage(msg);
}

bool OSCManager::sendMultiFloat(const String& address, float* values, int count) {
    if (!isEnabled() || count <= 0) {
        return false;
    }

    OSCMessage msg(address.c_str());
    for (int i = 0; i < count; i++) {
        msg.add(values[i]);
    }

    return sendOSCMessage(msg);
}

void OSCManager::setTarget(const String& target_ip, uint16_t target_port) {
    targetIP = target_ip;
    targetPort = target_port;
    
    debug_network( "[OSC] Nouvelle destination: %s:%d\n", 
                  targetIP.c_str(), targetPort);
}

String OSCManager::getTargetIP() const {
    return targetIP;
}

uint16_t OSCManager::getTargetPort() const {
    return targetPort;
}

bool OSCManager::isInitialized() const {
    return initialized;
}

void OSCManager::setBroadcast(bool enable) {
    broadcastEnabled = enable;
    debug_network( "[OSC] Broadcast %s\n", enable ? "activé" : "désactivé\n");
    if (enable && !broadcastIP.isEmpty()) {
        debug_network( "[OSC] Broadcast vers: %s:%d\n", broadcastIP.c_str(), targetPort);
    }
}

void OSCManager::setInterface(uint8_t interface) {
    networkInterface = interface;
    const char* interfaceNames[] = {"AP", "STA", "BOTH"};
    debug_network( "[OSC] Interface réseau configurée: %s\n", 
                 interface < 3 ? interfaceNames[interface] : "INVALID\n");
}

uint8_t OSCManager::getInterface() const {
    return networkInterface;
}

bool OSCManager::isBroadcastEnabled() const {
    return broadcastEnabled;
}

bool OSCManager::sendOSCMessage(OSCMessage& msg) {
    if (!isEnabled()) {
        debug_network( "[OSC] OSC désactivé\n");
        return false;
    }

    bool success = false;
    int retryCount = 0;
    const int maxRetries = 2;
    
    // Mode broadcast OU IP spécifique selon l'interface configurée
    if (broadcastEnabled) {
        // Mode broadcast selon l'interface réseau
        if (networkInterface == OSC_INTERFACE_AP || networkInterface == OSC_INTERFACE_BOTH) {
            // Broadcast sur AP (192.168.4.255)
            while (retryCount <= maxRetries && !success) {
                if (udp.beginPacket("192.168.4.255", targetPort)) {
                    msg.send(udp);
                    if (udp.endPacket()) {
                        success = true;
                        debug_network( "[OSC] Broadcast AP réussi (tentative %d)\n", retryCount + 1);
                    } else {
                        debug_network( "[OSC] Échec endPacket AP (tentative %d)\n", retryCount + 1);
                    }
                } else {
                    debug_network( "[OSC] Échec beginPacket AP (tentative %d)\n", retryCount + 1);
                }
                retryCount++;
            }
        }
        
        if ((networkInterface == OSC_INTERFACE_STA || networkInterface == OSC_INTERFACE_BOTH) && 
            WiFi.status() == WL_CONNECTED) {
            // Broadcast sur STA (calculé selon le réseau)
            IPAddress ip = WiFi.localIP();
            IPAddress subnet = WiFi.subnetMask();
            IPAddress broadcast = IPAddress(ip[0] | (~subnet[0]), 
                                           ip[1] | (~subnet[1]), 
                                           ip[2] | (~subnet[2]), 
                                           ip[3] | (~subnet[3]));
            
            retryCount = 0;
            while (retryCount <= maxRetries && !success) {
                if (udp.beginPacket(broadcast, targetPort)) {
                    msg.send(udp);
                    if (udp.endPacket()) {
                        success = true;
                        debug_network( "[OSC] Broadcast STA réussi (tentative %d)\n", retryCount + 1);
                    } else {
                        debug_network( "[OSC] Échec endPacket STA (tentative %d)\n", retryCount + 1);
                    }
                } else {
                    debug_network( "[OSC] Échec beginPacket STA (tentative %d)\n", retryCount + 1);
                }
                retryCount++;
            }
        }
    } else {
        // Mode IP spécifique
        if (!targetIP.isEmpty()) {
            while (retryCount <= maxRetries && !success) {
                if (udp.beginPacket(targetIP.c_str(), targetPort)) {
                    msg.send(udp);
                    if (udp.endPacket()) {
                        success = true;
                        debug_network( "[OSC] Unicast réussi (tentative %d)\n", retryCount + 1);
                    } else {
                        debug_network( "[OSC] Échec endPacket unicast (tentative %d)\n", retryCount + 1);
                    }
                } else {
                    debug_network( "[OSC] Échec beginPacket unicast (tentative %d)\n", retryCount + 1);
                }
                retryCount++;
            }
        }
    }
    
    if (!success) {
        debug_network( "[OSC] Échec définitif après %d tentatives\n", maxRetries + 1);
    }
    
    return success;
}

void OSCManager::update() {
    if (!isEnabled() || !initialized) {
        return;
    }

    // Vérifier s'il y a des paquets entrants
    int packetSize = udp.parsePacket();
    if (packetSize > 0) {
        // Lire le paquet entrant
        uint8_t buffer[256];
        int len = udp.read(buffer, sizeof(buffer) - 1);
        if (len > 0) {
            buffer[len] = 0; // Null terminate
            
            // Parser le message OSC
            OSCMessage msg;
            msg.fill(buffer, len);
            
            if (!msg.hasError()) {
                // Extraire l'adresse
                char addressBuffer[64];
                msg.getAddress(addressBuffer, sizeof(addressBuffer));
                String address = String(addressBuffer);
                
                // Extraire la première valeur float (si disponible)
                if (msg.size() > 0 && msg.isFloat(0)) {
                    float value = msg.getFloat(0);
                    
                    // Appeler le callback si défini
                    if (messageCallback) {
                        messageCallback(address, value);
                    }
                }
            }
        }
    }
}

void OSCManager::setMessageCallback(OSCMessageCallback callback) {
    messageCallback = callback;
    #ifdef ESP32SERVER_debug_osc
    debug_network( "[OSC] Callback %s\n", callback ? "défini" : "supprimé\n");
    #endif
}

void OSCManager::printStatus() const {
    debug_network( "=== OSC Manager Status ===\n");
    debug_network( "Initialisé: %s\n", initialized ? "Oui" : "Non\n");
    debug_network( "Activé: %s\n", enabled ? "Oui" : "Non\n");
    debug_network( "Destination: %s:%d\n", targetIP.c_str(), targetPort);
    debug_network( "Broadcast: %s\n", broadcastEnabled ? "Oui" : "Non\n");
    debug_network( "Callback: %s\n", messageCallback ? "Défini" : "Non défini\n");
    debug_network( "=========================\n");
}

void OSCManager::disconnect() {
    // UDP n'a pas besoin de déconnexion explicite
    debug_network("[OSC] UDP arrêté\n");
}
