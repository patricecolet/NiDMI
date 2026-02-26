#pragma once

#include <Arduino.h>
#include "I2CManager.h"

/**
 * @file Mpr121Driver.h
 * @brief Driver pour le capteur capacitif MPR121 (12 électrodes, I2C)
 *
 * Adresses I2C : 0x5A (ADDR=GND), 0x5B (ADDR=VDD), 0x5C (ADDR=SDA), 0x5D (ADDR=SCL).
 * Lit le statut touch (12 bits) depuis les registres 0x00 et 0x01.
 */
class Mpr121Driver {
public:
    // Adresses I2C possibles (7-bit)
    static constexpr uint8_t ADDRESS_5A = 0x5A;  // ADDR = GND
    static constexpr uint8_t ADDRESS_5B = 0x5B;   // ADDR = VDD
    static constexpr uint8_t ADDRESS_5C = 0x5C;   // ADDR = SDA
    static constexpr uint8_t ADDRESS_5D = 0x5D;   // ADDR = SCL

    // Registres MPR121
    static constexpr uint8_t REG_TOUCH_STATUS_L = 0x00;
    static constexpr uint8_t REG_TOUCH_STATUS_H = 0x01;
    static constexpr uint8_t REG_OOR_STATUS_L   = 0x02;  // Out-of-range status
    static constexpr uint8_t REG_OOR_STATUS_H   = 0x03;

    // Filtrage baseline (rising = release, falling = touch, touched)
    static constexpr uint8_t REG_MHD_R = 0x2B;
    static constexpr uint8_t REG_NHD_R = 0x2C;
    static constexpr uint8_t REG_NCL_R = 0x2D;
    static constexpr uint8_t REG_FDL_R = 0x2E;
    static constexpr uint8_t REG_MHD_F = 0x2F;
    static constexpr uint8_t REG_NHD_F = 0x30;
    static constexpr uint8_t REG_NCL_F = 0x31;
    static constexpr uint8_t REG_FDL_F = 0x32;
    static constexpr uint8_t REG_NHD_T = 0x33;
    static constexpr uint8_t REG_NCL_T = 0x34;
    static constexpr uint8_t REG_FDL_T = 0x35;

    // Seuils touch/release (2 octets par électrode, 0x41-0x58)
    static constexpr uint8_t REG_E0_TTH = 0x41;

    // Debounce
    static constexpr uint8_t REG_DEBOUNCE = 0x5B;

    // Filtrage global et charge/décharge
    static constexpr uint8_t REG_CONFIG1 = 0x5C;  // FFI, CDC
    static constexpr uint8_t REG_CONFIG2 = 0x5D;  // CDT, SFI, ESI

    // Electrode configuration (doit être écrit en dernier)
    static constexpr uint8_t REG_ECR = 0x5E;

    // Auto-configuration
    static constexpr uint8_t REG_AUTO_CFG0 = 0x7B;
    static constexpr uint8_t REG_AUTO_CFG1 = 0x7C;
    static constexpr uint8_t REG_USL       = 0x7D;  // Up-side limit
    static constexpr uint8_t REG_LSL       = 0x7E;  // Low-side limit
    static constexpr uint8_t REG_TL        = 0x7F;  // Target level

    static constexpr uint8_t REG_SOFT_RESET = 0x80;

    /**
     * @brief Constructeur
     * @param address Adresse I2C 7-bit (0x5A, 0x5B, 0x5C ou 0x5D)
     */
    explicit Mpr121Driver(uint8_t address);

    /**
     * @brief Initialise le bus I2C et le capteur
     * @param touch_thresh Seuil touch (1-50, défaut 6)
     * @param release_thresh Seuil release (1-50, doit être < touch, défaut 3)
     * @return true si l'initialisation a réussi
     */
    bool begin(uint8_t touch_thresh = 6, uint8_t release_thresh = 3);

    /**
     * @brief Vérifie si le MPR121 répond sur le bus
     */
    bool isConnected();

    /**
     * @brief Lit le statut des 12 électrodes (1 = touché)
     * @param out_mask Masque 12 bits (bit 0 = ELE0, ..., bit 11 = ELE11)
     * @return true si la lecture a réussi
     */
    bool readTouchStatus(uint16_t& out_mask);

    /**
     * @brief Lit les valeurs brutes (filtered) et baseline d'une électrode
     * @param electrode Index 0-11
     * @param filtered Valeur filtrée 10 bits (delta = baseline - filtered quand touché)
     * @param baseline Valeur baseline (8 bits, décalé <<2 en interne = 10 bits)
     */
    bool readElectrodeData(uint8_t electrode, uint16_t& filtered, uint16_t& baseline);

    /**
     * @brief Log diagnostic des 12 électrodes (filtered, baseline, delta)
     */
    void logDiagnostic();

private:
    uint8_t address_;

    uint8_t readRegister(uint8_t reg);
    bool writeRegister(uint8_t reg, uint8_t value);
};
