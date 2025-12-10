#pragma once

#include <Arduino.h>
#include "DebugManager.h"

// Types de MCU supportés
enum class McuType : uint8_t {
    ESP32_C3 = 0,
    ESP32_S3 = 1,
    UNKNOWN = 255
};

/**
 * @brief Pin physique avec ses capacités réelles
 * 
 * Représente une pin physique du MCU avec son GPIO et ses capacités.
 * C'est la source de vérité unique pour les capacités.
 */
struct PhysicalPin {
    uint8_t gpio;           // Numéro GPIO
    bool has_adc;           // Capacité ADC
    bool has_pwm;           // Capacité PWM
    bool has_touch;         // Capacité Touch
    const char* primary_label; // Label principal (ex: "D0", "D1")
};

/**
 * @brief Alias vers une pin physique
 * 
 * Permet de référencer une pin physique par un autre nom.
 * Ex: "A0" → D0, "SDA" → D4, "T1" → D0
 */
struct PinAlias {
    const char* alias;      // Nom de l'alias (ex: "A0", "SDA", "T1")
    uint8_t gpio;           // GPIO de la pin physique cible
};

/**
 * @brief Structure de compatibilité pour l'API existante
 * 
 * Utilisée par getAllMappings() pour maintenir la compatibilité
 * avec le code existant qui attend PinMapping.
 */
struct PinMapping {
    const char* label;
    uint8_t gpio;
    bool has_adc;
    bool has_pwm;
    bool has_touch;
};

/**
 * @brief Mapper de pins automatique selon le MCU
 * 
 * Architecture robuste : pins physiques + aliases
 * - Source de vérité unique pour les capacités (PhysicalPin)
 * - Aliases pointent vers les pins physiques
 * - Pas d'incohérence possible
 * 
 * Détecte automatiquement le type de MCU (ESP32-C3 ou ESP32-S3)
 * et fournit le mapping correct des pins.
 */
class PinMapper {
private:
    static McuType detected_mcu;
    static bool mcu_detected;
    
    // Pins physiques ESP32-C3 (XIAO-ESP32C3)
    static const PhysicalPin c3_physical_pins[];
    static const size_t c3_physical_pin_count;
    
    // Aliases ESP32-C3
    static const PinAlias c3_aliases[];
    static const size_t c3_alias_count;
    
    // Pins physiques ESP32-S3 (XIAO-ESP32S3)
    static const PhysicalPin s3_physical_pins[];
    static const size_t s3_physical_pin_count;
    
    // Aliases ESP32-S3
    static const PinAlias s3_aliases[];
    static const size_t s3_alias_count;
    
    // Fonctions internes pour résoudre les aliases
    static const PhysicalPin* findPhysicalPin(uint8_t gpio);
    static uint8_t resolveAlias(const char* label);
    
public:
    // Détection automatique du MCU
    static McuType detectMcu();
    
    // Mapping label → GPIO (résout les aliases automatiquement)
    static uint8_t labelToGpio(const String& label);
    static uint8_t labelToGpio(const char* label);
    
    // Mapping GPIO → label (retourne le label principal)
    static String gpioToLabel(uint8_t gpio);
    
    // Vérification des capacités (basée sur les pins physiques uniquement)
    static bool hasAdc(uint8_t gpio);
    static bool hasPwm(uint8_t gpio);
    static bool hasTouch(uint8_t gpio);
    
    // Getters
    static McuType getMcuType() { return detected_mcu; }
    static String getMcuName();
    
    // API de compatibilité (génère PinMapping depuis PhysicalPin + Aliases)
    static const PinMapping* getAllMappings();
    static size_t getMappingCount();
    
    // Debug
    static void printMappings();
};
