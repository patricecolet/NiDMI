#include "OSCQueue.h"
#include <WiFi.h>

OSCQueue::OSCQueue() 
    : messageQueue(nullptr), targetPort(8000), initialized(false), 
      broadcastEnabled(false), networkInterface(0), sentCount(0), failedCount(0) {
}

OSCQueue::~OSCQueue() {
    end();
}

bool OSCQueue::begin() {
    if (initialized) {
        return true;
    }
    
    // Créer la queue FreeRTOS
    messageQueue = xQueueCreate(QUEUE_SIZE, sizeof(OSCMessageItem));
    if (!messageQueue) {
        Serial.println("[OSCQueue] Erreur: Impossible de créer la queue");
        return false;
    }
    
    // Configuration UDP optimisée pour la fiabilité
    udp.setTimeout(1000); // Timeout 1s pour éviter les blocages
    // Démarrer UDP avec port différent pour éviter les conflits
    if (!udp.begin(4001)) { // Port différent de OSCManager (4000)
        Serial.println("[OSCQueue] Erreur: Impossible de démarrer UDP");
        vQueueDelete(messageQueue);
        messageQueue = nullptr;
        return false;
    }
    
    // Configuration WiFi optimisée pour la fiabilité
    WiFi.setSleep(false); // Désactiver le sleep WiFi pour éviter les pertes
    WiFi.setAutoReconnect(true); // Reconnexion automatique
    
    initialized = true;
    // Serial.println("[OSCQueue] Initialisé avec succès (port 4001)");
    return true;
}

void OSCQueue::end() {
    if (messageQueue) {
        vQueueDelete(messageQueue);
        messageQueue = nullptr;
    }
    udp.stop();
    initialized = false;
    // Serial.println("[OSCQueue] Arrêté");
}

bool OSCQueue::enqueueFloat(const String& address, float value) {
    if (!initialized || !messageQueue) {
        return false;
    }
    
    OSCMessageItem item;
    item.address = address;
    item.value = value;
    item.data1 = 0;
    item.data2 = 0;
    item.channel = 0;
    item.messageType = 0; // Float
    item.timestamp = millis();
    
    BaseType_t result = xQueueSend(messageQueue, &item, 0); // Non-bloquant
    if (result != pdTRUE) {
        // Serial.printf("[OSCQueue] Queue pleine, message float perdu: %s=%.3f\n", 
        //              address.c_str(), value);
        return false;
    }
    
    return true;
}

bool OSCQueue::enqueueMidi(const String& address, uint8_t data1, uint8_t data2, uint8_t channel) {
    if (!initialized || !messageQueue) {
        return false;
    }
    
    OSCMessageItem item;
    item.address = address;
    item.value = 0.0f;
    item.data1 = data1;
    item.data2 = data2;
    item.channel = channel;
    item.messageType = 1; // MIDI
    item.timestamp = millis();
    
    BaseType_t result = xQueueSend(messageQueue, &item, 0); // Non-bloquant
    if (result != pdTRUE) {
        // Serial.printf("[OSCQueue] Queue pleine, message MIDI perdu: %s ch%d d1%d d2%d\n", 
        //              address.c_str(), channel, data1, data2);
        return false;
    }
    
    return true;
}

void OSCQueue::update() {
    if (!initialized || !messageQueue) {
        return;
    }
    
    // Traiter jusqu'à 3 messages par cycle pour éviter de bloquer
    for (int i = 0; i < 3; i++) {
        OSCMessageItem item;
        BaseType_t result = xQueueReceive(messageQueue, &item, 0); // Non-bloquant
        
        if (result != pdTRUE) {
            break; // Pas de message en attente
        }
        
        // Créer et envoyer le message OSC
        OSCMessage msg(item.address.c_str());
        
        if (item.messageType == 0) { // Float
            msg.add(item.value);
        } else { // MIDI
            msg.add((int32_t)item.data1);
            msg.add((int32_t)item.data2);
            msg.add((int32_t)item.channel);
        }
        
        if (sendOSCMessage(msg)) {
            sentCount++;
        } else {
            failedCount++;
        }
    }
}

void OSCQueue::setTarget(const String& target_ip, uint16_t target_port) {
    targetIP = target_ip;
    targetPort = target_port;
    // Serial.printf("[OSCQueue] Cible: %s:%d\n", targetIP.c_str(), targetPort);
}

void OSCQueue::setBroadcast(bool enable) {
    broadcastEnabled = enable;
    // Serial.printf("[OSCQueue] Broadcast: %s\n", enable ? "activé" : "désactivé");
}

void OSCQueue::setInterface(uint8_t interface) {
    networkInterface = interface;
    // Serial.printf("[OSCQueue] Interface: %d\n", interface);
}

uint32_t OSCQueue::getQueueSize() const {
    if (!messageQueue) return 0;
    return uxQueueMessagesWaiting(messageQueue);
}

uint32_t OSCQueue::getSentCount() const {
    return sentCount;
}

uint32_t OSCQueue::getFailedCount() const {
    return failedCount;
}

void OSCQueue::resetStats() {
    sentCount = 0;
    failedCount = 0;
}

void OSCQueue::printNetworkStatus() const {
    Serial.println("=== OSCQueue Network Status ===");
    Serial.printf("WiFi Status: %d (%s)\n", WiFi.status(), 
                  WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("Local IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("Subnet: %s\n", WiFi.subnetMask().toString().c_str());
        Serial.printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
        Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
    }
    Serial.printf("Target: %s:%d\n", targetIP.c_str(), targetPort);
    Serial.printf("Broadcast: %s\n", broadcastEnabled ? "Enabled" : "Disabled");
    Serial.printf("Interface: %d (0=AP, 1=STA, 2=BOTH)\n", networkInterface);
    Serial.printf("Queue Size: %d/%d\n", getQueueSize(), QUEUE_SIZE);
    Serial.println("===============================");
}

void OSCQueue::printDetailedStats() const {
    Serial.println("=== OSCQueue Detailed Stats ===");
    Serial.printf("Messages sent: %d\n", sentCount);
    Serial.printf("Messages failed: %d\n", failedCount);
    Serial.printf("Success rate: %.1f%%\n", 
                  sentCount + failedCount > 0 ? 
                  (float)sentCount / (sentCount + failedCount) * 100.0f : 0.0f);
    Serial.printf("Queue utilization: %.1f%%\n", 
                  (float)getQueueSize() / QUEUE_SIZE * 100.0f);
    Serial.println("===============================");
}

bool OSCQueue::sendOSCMessage(OSCMessage& msg) {
    bool success = false;
    int retryCount = 0;
    const int maxRetries = 3; // Plus de retry pour la fiabilité
    
    // Vérifier l'état WiFi avant l'envoi
    if (WiFi.status() != WL_CONNECTED && networkInterface != 0) {
        // Serial.printf("[OSCQueue] WiFi déconnecté, statut: %d\n", WiFi.status());
        return false;
    }
    
    if (broadcastEnabled) {
        // Mode broadcast avec optimisation
        if (networkInterface == 0 || networkInterface == 2) { // AP ou BOTH
            while (retryCount <= maxRetries && !success) {
                if (udp.beginPacket("192.168.4.255", targetPort)) {
                    msg.send(udp);
                    if (udp.endPacket()) {
                        success = true;
                        // Serial.printf("[OSCQueue] Broadcast AP réussi (tentative %d)\n", retryCount + 1);
                    } else {
                        // Serial.printf("[OSCQueue] Échec endPacket AP (tentative %d)\n", retryCount + 1);
                    }
                } else {
                    // Serial.printf("[OSCQueue] Échec beginPacket AP (tentative %d)\n", retryCount + 1);
                }
                retryCount++;
                if (!success && retryCount <= maxRetries) {
                    delay(2); // Petit délai entre les tentatives
                }
            }
        }
        
        if ((networkInterface == 1 || networkInterface == 2) && WiFi.status() == WL_CONNECTED) {
            // Broadcast STA avec calcul d'adresse optimisé
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
                        // Serial.printf("[OSCQueue] Broadcast STA réussi (tentative %d)\n", retryCount + 1);
                    } else {
                        // Serial.printf("[OSCQueue] Échec endPacket STA (tentative %d)\n", retryCount + 1);
                    }
                } else {
                    // Serial.printf("[OSCQueue] Échec beginPacket STA (tentative %d)\n", retryCount + 1);
                }
                retryCount++;
                if (!success && retryCount <= maxRetries) {
                    delay(2); // Petit délai entre les tentatives
                }
            }
        }
    } else {
        // Mode unicast avec vérification d'adresse
        if (!targetIP.isEmpty()) {
            while (retryCount <= maxRetries && !success) {
                if (udp.beginPacket(targetIP.c_str(), targetPort)) {
                    msg.send(udp);
                    if (udp.endPacket()) {
                        success = true;
                        // Serial.printf("[OSCQueue] Unicast réussi (tentative %d)\n", retryCount + 1);
                    } else {
                        // Serial.printf("[OSCQueue] Échec endPacket unicast (tentative %d)\n", retryCount + 1);
                    }
                } else {
                    // Serial.printf("[OSCQueue] Échec beginPacket unicast (tentative %d)\n", retryCount + 1);
                }
                retryCount++;
                if (!success && retryCount <= maxRetries) {
                    delay(2); // Petit délai entre les tentatives
                }
            }
        }
    }
    
    if (!success) {
        // Serial.printf("[OSCQueue] Échec définitif après %d tentatives\n", maxRetries + 1);
    }
    
    return success;
}
