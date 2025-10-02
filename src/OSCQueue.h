#ifndef OSCQUEUE_H
#define OSCQUEUE_H

#include <Arduino.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// Structure pour les messages OSC en queue
struct OSCMessageItem {
    String address;
    float value;
    uint8_t data1;
    uint8_t data2;
    uint8_t channel;
    uint8_t messageType; // 0=float, 1=MIDI
    uint32_t timestamp;
};

class OSCQueue {
public:
    OSCQueue();
    ~OSCQueue();

    bool begin();
    void end();
    
    // Ajouter des messages à la queue (non-bloquant)
    bool enqueueFloat(const String& address, float value);
    bool enqueueMidi(const String& address, uint8_t data1, uint8_t data2, uint8_t channel);
    
    // Traiter la queue (à appeler dans loop())
    void update();
    
    // Configuration
    void setTarget(const String& target_ip, uint16_t target_port);
    void setBroadcast(bool enable);
    void setInterface(uint8_t interface);
    
    // Statistiques
    uint32_t getQueueSize() const;
    uint32_t getSentCount() const;
    uint32_t getFailedCount() const;
    void resetStats();
    
    // Diagnostic réseau
    void printNetworkStatus() const;
    void printDetailedStats() const;

private:
    void processQueue();
    bool sendOSCMessage(OSCMessage& msg);
    
private:
    QueueHandle_t messageQueue;
    WiFiUDP udp;
    String targetIP;
    uint16_t targetPort;
    bool initialized;
    bool broadcastEnabled;
    uint8_t networkInterface;
    
    // Statistiques
    uint32_t sentCount;
    uint32_t failedCount;
    
    static const int QUEUE_SIZE = 32;
    static const int MAX_RETRIES = 2;
};

#endif // OSCQUEUE_H
