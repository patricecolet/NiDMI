#include "I2CManager.h"
#include "../utils/PinMapper.h"

bool I2CManager::initialized_ = false;
uint8_t I2CManager::sda_pin_ = 255;
uint8_t I2CManager::scl_pin_ = 255;

bool I2CManager::begin(uint8_t sda, uint8_t scl, uint32_t frequency) {
    if (initialized_) {
        return true; // Déjà initialisé
    }
    
    // Utiliser les pins par défaut du MCU si non spécifiées
    if (sda == 255 || scl == 255) {
        // Récupérer les pins I2C depuis PinMapper
        // Note: PinMapper devrait avoir une méthode pour obtenir les pins I2C
        // Pour l'instant, on utilise les valeurs par défaut selon le MCU
        #if defined(CONFIG_IDF_TARGET_ESP32C3)
            sda = 6;  // GPIO6 pour ESP32-C3
            scl = 7;  // GPIO7 pour ESP32-C3
        #elif defined(CONFIG_IDF_TARGET_ESP32S3)
            sda = 4;  // GPIO4 pour ESP32-S3
            scl = 5;  // GPIO5 pour ESP32-S3
        #else
            sda = 21; // Défaut ESP32
            scl = 22;
        #endif
    }
    
    sda_pin_ = sda;
    scl_pin_ = scl;
    
    Wire.begin(sda, scl);
    Wire.setClock(frequency);
    
    initialized_ = true;
    Serial.printf("[I2CManager] I2C initialisé sur SDA=%d, SCL=%d, freq=%d Hz\n", sda, scl, frequency);
    
    return true;
}

bool I2CManager::deviceExists(uint8_t address) {
    if (!initialized_) {
        return false;
    }
    
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();
    return (error == 0);
}

uint8_t I2CManager::readRegister(uint8_t address, uint8_t reg) {
    if (!initialized_) {
        return 0xFF;
    }
    
    Wire.beginTransmission(address);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) {
        return 0xFF;
    }
    
    if (Wire.requestFrom(address, (uint8_t)1) != 1) {
        return 0xFF;
    }
    
    return Wire.read();
}

bool I2CManager::writeRegister(uint8_t address, uint8_t reg, uint8_t value) {
    if (!initialized_) {
        return false;
    }
    
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(value);
    return (Wire.endTransmission() == 0);
}

bool I2CManager::readRegisters(uint8_t address, uint8_t reg, uint8_t* data, size_t length) {
    if (!initialized_ || !data || length == 0) {
        return false;
    }
    
    Wire.beginTransmission(address);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) {
        return false;
    }
    
    if (Wire.requestFrom(address, (uint8_t)length) != length) {
        return false;
    }
    
    for (size_t i = 0; i < length; i++) {
        data[i] = Wire.read();
    }
    
    return true;
}
