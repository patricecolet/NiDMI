#pragma once

#include <Arduino.h>
#include <Wire.h>

/**
 * @file I2CManager.h
 * @brief Gestionnaire I2C centralisé pour tous les périphériques I2C
 * 
 * Singleton qui initialise le bus I2C une seule fois et fournit
 * des méthodes utilitaires pour la communication I2C.
 */
class I2CManager {
public:
    /**
     * @brief Initialise le bus I2C
     * @param sda Pin SDA (utilise les pins par défaut du MCU si 255)
     * @param scl Pin SCL (utilise les pins par défaut du MCU si 255)
     * @param frequency Fréquence I2C en Hz (défaut: 100000)
     * @return true si l'initialisation a réussi
     */
    static bool begin(uint8_t sda = 255, uint8_t scl = 255, uint32_t frequency = 100000);
    
    /**
     * @brief Vérifie si un périphérique répond à une adresse I2C
     * @param address Adresse I2C (7-bit)
     * @return true si le périphérique répond
     */
    static bool deviceExists(uint8_t address);
    
    /**
     * @brief Lit un registre 8-bit depuis un périphérique I2C
     * @param address Adresse I2C (7-bit)
     * @param reg Adresse du registre
     * @return Valeur du registre ou 0xFF en cas d'erreur
     */
    static uint8_t readRegister(uint8_t address, uint8_t reg);
    
    /**
     * @brief Écrit un registre 8-bit vers un périphérique I2C
     * @param address Adresse I2C (7-bit)
     * @param reg Adresse du registre
     * @param value Valeur à écrire
     * @return true si l'écriture a réussi
     */
    static bool writeRegister(uint8_t address, uint8_t reg, uint8_t value);
    
    /**
     * @brief Lit plusieurs registres consécutifs depuis un périphérique I2C
     * @param address Adresse I2C (7-bit)
     * @param reg Adresse du premier registre
     * @param data Buffer de réception
     * @param length Nombre d'octets à lire
     * @return true si la lecture a réussi
     */
    static bool readRegisters(uint8_t address, uint8_t reg, uint8_t* data, size_t length);
    
    /**
     * @brief Vérifie si le bus I2C est initialisé
     * @return true si initialisé
     */
    static bool isInitialized() { return initialized_; }
    
private:
    static bool initialized_;
    static uint8_t sda_pin_;
    static uint8_t scl_pin_;
};
