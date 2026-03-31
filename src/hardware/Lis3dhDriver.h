#pragma once

#include <Arduino.h>
#include "I2CManager.h"
#include <SPI.h>

/**
 * @file Lis3dhDriver.h
 * @brief Driver pour l'accéléromètre LIS3DH (3 axes)
 *
 * Support I2C et SPI. En I2C : adresses 0x18 (SA0=LOW) ou 0x19 (SA0=HIGH).
 * En SPI : Mode 0, MSB first, CS câblé à GND (toujours sélectionné).
 */
class Lis3dhDriver {
public:
    // Adresses I2C possibles
    static constexpr uint8_t ADDRESS_LOW = 0x18;   // SA0 = LOW
    static constexpr uint8_t ADDRESS_HIGH = 0x19;   // SA0 = HIGH

    // Masques SPI (LIS3DH datasheet)
    static constexpr uint8_t SPI_READ_BIT = 0x80;
    static constexpr uint8_t SPI_AUTO_INCREMENT = 0x40;
    
    // Registres principaux
    static constexpr uint8_t REG_WHO_AM_I = 0x0F;
    static constexpr uint8_t REG_CTRL_REG1 = 0x20;
    static constexpr uint8_t REG_CTRL_REG4 = 0x23;
    static constexpr uint8_t REG_OUT_X_L = 0x28;
    static constexpr uint8_t REG_OUT_X_H = 0x29;
    static constexpr uint8_t REG_OUT_Y_L = 0x2A;
    static constexpr uint8_t REG_OUT_Y_H = 0x2B;
    static constexpr uint8_t REG_OUT_Z_L = 0x2C;
    static constexpr uint8_t REG_OUT_Z_H = 0x2D;
    
    // Valeur WHO_AM_I attendue
    static constexpr uint8_t WHO_AM_I_VALUE = 0x33;
    
    /**
     * @brief Structure pour stocker les données d'accélération
     */
    struct AccelerationData {
        int16_t x;  // Accélération axe X (en unités LSB)
        int16_t y;  // Accélération axe Y (en unités LSB)
        int16_t z;  // Accélération axe Z (en unités LSB)
    };
    
    /**
     * @brief Plages de mesure disponibles
     */
    enum class Range : uint8_t {
        RANGE_2G = 0,   // ±2g
        RANGE_4G = 1,   // ±4g
        RANGE_8G = 2,   // ±8g
        RANGE_16G = 3   // ±16g
    };
    
    /**
     * @brief Taux de données disponibles
     */
    enum class DataRate : uint8_t {
        RATE_1HZ = 0x10,
        RATE_10HZ = 0x20,
        RATE_25HZ = 0x30,
        RATE_50HZ = 0x40,
        RATE_100HZ = 0x50,
        RATE_200HZ = 0x60,
        RATE_400HZ = 0x70,
        RATE_1_6KHZ = 0x80,  // Mode basse consommation
        RATE_5KHZ = 0x90     // Mode haute résolution
    };
    
    /**
     * @brief Constructeur I2C
     * @param address Adresse I2C (ADDRESS_LOW ou ADDRESS_HIGH)
     */
    explicit Lis3dhDriver(uint8_t address);

    /**
     * @brief Constructeur SPI (utiliser beginSPI après)
     */
    Lis3dhDriver();

    /**
     * @brief Initialise le capteur en I2C (appeler si construit avec adresse)
     * @return true si l'initialisation a réussi
     */
    bool begin();

    /**
     * @brief Initialise le capteur en SPI
     * @param sck GPIO SCK, @param miso GPIO MISO, @param mosi GPIO MOSI
     * @param cs GPIO Chip Select (doit basculer pour cadrer les transactions)
     * @param spi_freq_hz Fréquence SPI en Hz (défaut 1 MHz)
     * @return true si l'initialisation a réussi
     */
    bool beginSPI(uint8_t sck, uint8_t miso, uint8_t mosi, uint8_t cs, uint32_t spi_freq_hz = 1000000);
    
    /**
     * @brief Vérifie si le capteur est présent et répond
     * @return true si le capteur répond
     */
    bool isConnected();
    
    /**
     * @brief Configure la plage de mesure
     * @param range Plage de mesure
     * @return true si la configuration a réussi
     */
    bool setRange(Range range);
    
    /**
     * @brief Configure le taux de données
     * @param rate Taux de données
     * @return true si la configuration a réussi
     */
    bool setDataRate(DataRate rate);
    
    /**
     * @brief Active le capteur
     * @param enable true pour activer, false pour désactiver
     * @return true si la configuration a réussi
     */
    bool enable(bool enable = true);
    
    /**
     * @brief Lit les données d'accélération
     * @param data Structure pour stocker les données
     * @return true si la lecture a réussi
     */
    bool readAcceleration(AccelerationData& data);
    
    /**
     * @brief Obtient la plage actuelle en g
     * @return Plage en g (±2, ±4, ±8, ±16)
     */
    uint8_t getRangeInG() const { return range_in_g_; }
    
private:
    uint8_t address_;
    uint8_t range_in_g_;
    bool use_spi_;
    uint8_t cs_pin_;
    uint32_t spi_freq_hz_;

    /**
     * @brief Lit un registre 8-bit (I2C ou SPI selon use_spi_)
     */
    uint8_t readRegister(uint8_t reg);

    /**
     * @brief Écrit un registre 8-bit (I2C ou SPI selon use_spi_)
     */
    bool writeRegister(uint8_t reg, uint8_t value);

    uint8_t readRegisterSPI(uint8_t reg);
    bool writeRegisterSPI(uint8_t reg, uint8_t value);
};
