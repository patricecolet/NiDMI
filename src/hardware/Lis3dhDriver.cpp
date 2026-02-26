#include "Lis3dhDriver.h"

Lis3dhDriver::Lis3dhDriver(uint8_t address)
    : address_(address), range_in_g_(2), use_spi_(false), cs_pin_(255), spi_freq_hz_(1000000) {
}

Lis3dhDriver::Lis3dhDriver()
    : address_(0), range_in_g_(2), use_spi_(true), cs_pin_(255), spi_freq_hz_(1000000) {
}

bool Lis3dhDriver::begin() {
    if (use_spi_) {
        Serial.println("[Lis3dhDriver] Utilisez beginSPI() pour le mode SPI");
        return false;
    }
    if (!I2CManager::isInitialized()) {
        I2CManager::begin();
    }
    if (!isConnected()) {
        Serial.printf("[Lis3dhDriver] Capteur non détecté à l'adresse 0x%02X\n", address_);
        return false;
    }
    if (!setRange(Range::RANGE_2G)) return false;
    if (!setDataRate(DataRate::RATE_100HZ)) return false;
    if (!enable(true)) return false;
    Serial.printf("[Lis3dhDriver] LIS3DH I2C initialisé à 0x%02X\n", address_);
    return true;
}

bool Lis3dhDriver::beginSPI(uint8_t sck, uint8_t miso, uint8_t mosi, uint8_t cs, uint32_t spi_freq_hz) {
    use_spi_ = true;
    cs_pin_ = cs;
    spi_freq_hz_ = spi_freq_hz;
    pinMode(cs_pin_, OUTPUT);
    digitalWrite(cs_pin_, HIGH);
    SPI.begin(sck, miso, mosi, -1);
    if (!isConnected()) {
        Serial.printf("[Lis3dhDriver] Capteur SPI non détecté SCK=%d MISO=%d MOSI=%d CS=%d\n", (int)sck, (int)miso, (int)mosi, (int)cs);
        return false;
    }
    if (!setRange(Range::RANGE_2G)) return false;
    if (!setDataRate(DataRate::RATE_100HZ)) return false;
    if (!enable(true)) return false;
    Serial.printf("[Lis3dhDriver] LIS3DH SPI OK SCK=%d MISO=%d MOSI=%d CS=%d\n", (int)sck, (int)miso, (int)mosi, (int)cs);
    return true;
}

bool Lis3dhDriver::isConnected() {
    uint8_t who_am_i = readRegister(REG_WHO_AM_I);
    return (who_am_i == WHO_AM_I_VALUE);
}

bool Lis3dhDriver::setRange(Range range) {
    uint8_t ctrl_reg4 = readRegister(REG_CTRL_REG4);
    
    // Effacer les bits FS1 et FS0 (bits 5-4)
    ctrl_reg4 &= ~0x30;
    
    // Définir la nouvelle plage
    ctrl_reg4 |= (static_cast<uint8_t>(range) << 4);
    
    if (!writeRegister(REG_CTRL_REG4, ctrl_reg4)) {
        return false;
    }
    
    // Mémoriser la plage en g
    range_in_g_ = 2 << static_cast<uint8_t>(range);
    
    return true;
}

bool Lis3dhDriver::setDataRate(DataRate rate) {
    uint8_t ctrl_reg1 = readRegister(REG_CTRL_REG1);
    
    // Effacer les bits ODR (bits 7-4)
    ctrl_reg1 &= ~0xF0;
    
    // Définir le nouveau taux de données
    ctrl_reg1 |= static_cast<uint8_t>(rate);
    
    return writeRegister(REG_CTRL_REG1, ctrl_reg1);
}

bool Lis3dhDriver::enable(bool enable) {
    uint8_t ctrl_reg1 = readRegister(REG_CTRL_REG1);
    
    if (enable) {
        // Activer : bit 3 (LPen) = 0, bit 0 (Xen) = 1, bit 1 (Yen) = 1, bit 2 (Zen) = 1
        ctrl_reg1 &= ~0x08;  // LPen = 0
        ctrl_reg1 |= 0x07;   // Xen, Yen, Zen = 1
    } else {
        // Désactiver : désactiver tous les axes
        ctrl_reg1 &= ~0x07;
    }
    
    return writeRegister(REG_CTRL_REG1, ctrl_reg1);
}

bool Lis3dhDriver::readAcceleration(AccelerationData& data) {
    uint8_t raw_data[6];
    if (use_spi_) {
        SPI.beginTransaction(SPISettings(spi_freq_hz_, MSBFIRST, SPI_MODE0));
        digitalWrite(cs_pin_, LOW);
        SPI.transfer(REG_OUT_X_L | SPI_READ_BIT | SPI_AUTO_INCREMENT);
        for (int i = 0; i < 6; i++) {
            raw_data[i] = SPI.transfer(0);
        }
        digitalWrite(cs_pin_, HIGH);
        SPI.endTransaction();
    } else {
        if (!I2CManager::readRegisters(address_, REG_OUT_X_L, raw_data, 6)) {
            return false;
        }
    }
    data.x = (int16_t)((raw_data[1] << 8) | raw_data[0]);
    data.y = (int16_t)((raw_data[3] << 8) | raw_data[2]);
    data.z = (int16_t)((raw_data[5] << 8) | raw_data[4]);
    return true;
}

uint8_t Lis3dhDriver::readRegister(uint8_t reg) {
    if (use_spi_) return readRegisterSPI(reg);
    return I2CManager::readRegister(address_, reg);
}

bool Lis3dhDriver::writeRegister(uint8_t reg, uint8_t value) {
    if (use_spi_) return writeRegisterSPI(reg, value);
    return I2CManager::writeRegister(address_, reg, value);
}

uint8_t Lis3dhDriver::readRegisterSPI(uint8_t reg) {
    SPI.beginTransaction(SPISettings(spi_freq_hz_, MSBFIRST, SPI_MODE0));
    digitalWrite(cs_pin_, LOW);
    SPI.transfer(reg | SPI_READ_BIT);
    uint8_t val = SPI.transfer(0);
    digitalWrite(cs_pin_, HIGH);
    SPI.endTransaction();
    return val;
}

bool Lis3dhDriver::writeRegisterSPI(uint8_t reg, uint8_t value) {
    SPI.beginTransaction(SPISettings(spi_freq_hz_, MSBFIRST, SPI_MODE0));
    digitalWrite(cs_pin_, LOW);
    SPI.transfer(reg & ~SPI_READ_BIT);
    SPI.transfer(value);
    digitalWrite(cs_pin_, HIGH);
    SPI.endTransaction();
    return true;
}
