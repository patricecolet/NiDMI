#ifndef BLUETOOTHMANAGER_H
#define BLUETOOTHMANAGER_H

#include <Arduino.h>

// BLE MIDI conditionnel - seulement si activé
#ifdef ESP32SERVER_ENABLE_BLE_MIDI
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#endif

/**
 * @brief Gestionnaire BLE MIDI
 * 
 * Cette classe gère la communication MIDI via Bluetooth Low Energy (BLE).
 * Compatible avec les appareils iOS/Android et les contrôleurs MIDI BLE.
 */
class BluetoothManager {
private:
#ifdef ESP32SERVER_ENABLE_BLE_MIDI
    BLEServer* pServer;
    BLECharacteristic* pCharacteristic;
#endif
    String deviceName;
    bool isStarted;
    bool connected;
    uint32_t lastConnectionCheck;
    
public:
    BluetoothManager();
    ~BluetoothManager();
    
    // Initialisation
    bool begin(const String& name);
    void stop();
    void update();
    
    // Envoi MIDI
    void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
    void sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity);
    void sendControlChange(uint8_t channel, uint8_t control, uint8_t value);
    void sendProgramChange(uint8_t channel, uint8_t program);
    void sendPitchBend(uint8_t channel, int bend);
    
    // État de connexion
    bool isConnected() const;
    bool isInitialized() const { return isStarted; }
    String getName() const { return deviceName; }
    String getConnectedDevice() const;
    
    // Configuration
    void setDeviceName(const String& name);
    
    // Statistiques
    uint32_t getBytesSent() const;
    uint32_t getBytesReceived() const;
    void resetStats();
    
private:
    void sendMidiMessage(uint8_t status, uint8_t data1, uint8_t data2);
    void checkConnection();
    
    // Statistiques
    uint32_t bytesSent;
    uint32_t bytesReceived;
};

#endif // BLUETOOTHMANAGER_H
