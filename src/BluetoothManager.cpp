#include "BluetoothManager.h"

#ifdef ESP32SERVER_ENABLE_BLE_MIDI
// UUIDs pour BLE MIDI (plus simples)
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Callback pour les événements de connexion BLE
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        Serial.println("[BLE] Client connecté");
    };

    void onDisconnect(BLEServer* pServer) {
        Serial.println("[BLE] Client déconnecté");
        // Redémarrer la publicité pour permettre une nouvelle connexion
        pServer->startAdvertising();
    }
};

// Callback pour les données reçues
class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        
        if (value.length() > 0) {
            Serial.println("[BLE] Données reçues:");
            for (int i = 0; i < value.length(); i++) {
                Serial.printf("%02X ", (uint8_t)value[i]);
            }
            Serial.println();
        }
    }
};

BluetoothManager::BluetoothManager() 
#ifdef ESP32SERVER_ENABLE_BLE_MIDI
    : pServer(nullptr), pCharacteristic(nullptr), deviceName("ESP32-MIDI"), 
      isStarted(false), connected(false), lastConnectionCheck(0), 
      bytesSent(0), bytesReceived(0) {
#else
    : deviceName("ESP32-MIDI"), isStarted(false), connected(false), 
      lastConnectionCheck(0), bytesSent(0), bytesReceived(0) {
#endif
}

BluetoothManager::~BluetoothManager() {
    stop();
}

bool BluetoothManager::begin(const String& name) {
    if (isStarted) {
        return true;
    }
    
    deviceName = name;
    
    // Initialiser BLE
    BLEDevice::init(deviceName.c_str());
    
    // Créer le serveur BLE
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    
    // Créer le service BLE MIDI
    BLEService *pService = pServer->createService(SERVICE_UUID);
    
    // Créer la caractéristique MIDI
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    
    // Ajouter les callbacks
    pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
    
    // Ajouter le descripteur pour les notifications
    pCharacteristic->addDescriptor(new BLE2902());
    
    // Démarrer le service
    pService->start();
    
    // Configurer la publicité (approche simplifiée)
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->start();
    
    isStarted = true;
    Serial.printf("[BLE] Initialisé: %s\n", deviceName.c_str());
    Serial.println("[BLE] En attente de connexion...");
    
    return true;
}

void BluetoothManager::stop() {
    if (isStarted) {
        BLEDevice::deinit(true);
        pServer = nullptr;
        pCharacteristic = nullptr;
        isStarted = false;
        connected = false;
        Serial.println("[BLE] Arrêté");
    }
}

void BluetoothManager::update() {
    if (!isStarted) {
        return;
    }
    
    // Vérifier la connexion périodiquement
    checkConnection();
    
    // Traiter les données reçues (optionnel pour le futur)
    // BLE MIDI reçoit généralement des notifications
}

void BluetoothManager::sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (!connected || !pCharacteristic) {
        return;
    }
    
    sendMidiMessage(0x90 | (channel - 1), note, velocity);
}

void BluetoothManager::sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (!connected || !pCharacteristic) {
        return;
    }
    
    sendMidiMessage(0x80 | (channel - 1), note, velocity);
}

void BluetoothManager::sendControlChange(uint8_t channel, uint8_t control, uint8_t value) {
    if (!connected || !pCharacteristic) {
        return;
    }
    
    sendMidiMessage(0xB0 | (channel - 1), control, value);
}

void BluetoothManager::sendProgramChange(uint8_t channel, uint8_t program) {
    if (!connected || !pCharacteristic) {
        return;
    }
    
    sendMidiMessage(0xC0 | (channel - 1), program, 0);
}

void BluetoothManager::sendPitchBend(uint8_t channel, int bend) {
    if (!connected || !pCharacteristic) {
        return;
    }
    
    // Convertir de signé (-8192 à +8191) vers non-signé (0-16383)
    uint16_t bendValue = (uint16_t)(bend + 8192);
    uint8_t lsb = bendValue & 0x7F;
    uint8_t msb = (bendValue >> 7) & 0x7F;
    sendMidiMessage(0xE0 | (channel - 1), lsb, msb);
}

bool BluetoothManager::isConnected() const {
    return connected;
}

String BluetoothManager::getConnectedDevice() const {
    if (connected) {
        return "BLE Device"; // BLE ne fournit pas facilement cette info
    }
    return "";
}

void BluetoothManager::setDeviceName(const String& name) {
    deviceName = name;
    if (isStarted) {
        stop();
        begin(name);
    }
}

uint32_t BluetoothManager::getBytesSent() const {
    return bytesSent;
}

uint32_t BluetoothManager::getBytesReceived() const {
    return bytesReceived;
}

void BluetoothManager::resetStats() {
    bytesSent = 0;
    bytesReceived = 0;
}

void BluetoothManager::sendMidiMessage(uint8_t status, uint8_t data1, uint8_t data2) {
    if (!connected || !pCharacteristic) {
        return;
    }
    
    // Format BLE simple : MIDI data direct
    uint8_t midiData[3];
    midiData[0] = status;
    midiData[1] = data1;
    midiData[2] = data2;
    
    pCharacteristic->setValue(midiData, 3);
    pCharacteristic->notify();
    
    bytesSent += 3;
}

void BluetoothManager::checkConnection() {
    uint32_t now = millis();
    
    // Vérifier la connexion toutes les 5 secondes
    if (now - lastConnectionCheck > 5000) {
        bool wasConnected = connected;
        connected = pServer && pServer->getConnectedCount() > 0;
        
        if (connected && !wasConnected) {
            Serial.println("[BLE] Connexion établie");
        } else if (!connected && wasConnected) {
            Serial.println("[BLE] Connexion perdue");
        }
        
        lastConnectionCheck = now;
    }
}

#else
// Stubs pour quand BLE MIDI n'est pas activé
BluetoothManager::BluetoothManager() 
    : deviceName("ESP32-MIDI"), isStarted(false), connected(false), 
      lastConnectionCheck(0), bytesSent(0), bytesReceived(0) {
}

BluetoothManager::~BluetoothManager() {
    stop();
}

bool BluetoothManager::begin(const String& name) {
    deviceName = name;
    Serial.println("[BLE] BLE MIDI désactivé (compilation sans ESP32SERVER_ENABLE_BLE_MIDI)");
    return false;
}

void BluetoothManager::stop() {
    isStarted = false;
    connected = false;
}

void BluetoothManager::update() {
    // Rien à faire
}

void BluetoothManager::sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    // Rien à faire
}

void BluetoothManager::sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
    // Rien à faire
}

void BluetoothManager::sendControlChange(uint8_t channel, uint8_t control, uint8_t value) {
    // Rien à faire
}

void BluetoothManager::sendProgramChange(uint8_t channel, uint8_t program) {
    // Rien à faire
}

void BluetoothManager::sendPitchBend(uint8_t channel, int bend) {
    // Rien à faire
}

bool BluetoothManager::isConnected() const {
    return false;
}

String BluetoothManager::getConnectedDevice() const {
    return "";
}

void BluetoothManager::setDeviceName(const String& name) {
    deviceName = name;
}

uint32_t BluetoothManager::getBytesSent() const {
    return 0;
}

uint32_t BluetoothManager::getBytesReceived() const {
    return 0;
}

void BluetoothManager::resetStats() {
    bytesSent = 0;
    bytesReceived = 0;
}

void BluetoothManager::sendMidiMessage(uint8_t status, uint8_t data1, uint8_t data2) {
    // Rien à faire
}

void BluetoothManager::checkConnection() {
    // Rien à faire
}

#endif // ESP32SERVER_ENABLE_BLE_MIDI