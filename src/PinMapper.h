#pragma once

#include <Arduino.h>
#include "DebugManager.h"

// Types de MCU supportés
enum class McuType : uint8_t {
    ESP32_C3 = 0,
    ESP32_S3 = 1,
    UNKNOWN = 255
};

// Structure pour mapper label → GPIO
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
 * Détecte automatiquement le type de MCU (ESP32-C3 ou ESP32-S3)
 * et fournit le mapping correct des pins.
 */
class PinMapper {
private:
    static McuType detected_mcu;
    static bool mcu_detected;
    
    // Mappings pour ESP32-C3 (XIAO-ESP32C3)
    static const PinMapping c3_mappings[];
    static const size_t c3_mapping_count;
    
    // Mappings pour ESP32-S3 (XIAO-ESP32S3)
    static const PinMapping s3_mappings[];
    static const size_t s3_mapping_count;
    
public:
    // Détection automatique du MCU
    static McuType detectMcu();
    
    // Mapping label → GPIO
    static uint8_t labelToGpio(const String& label);
    static uint8_t labelToGpio(const char* label);
    
    // Mapping GPIO → label
    static String gpioToLabel(uint8_t gpio);
    
    // Vérification des capacités
    static bool hasAdc(uint8_t gpio);
    static bool hasPwm(uint8_t gpio);
    static bool hasTouch(uint8_t gpio);
    
    // Getters
    static McuType getMcuType() { return detected_mcu; }
    static String getMcuName();
    static const PinMapping* getAllMappings();
    static size_t getMappingCount();
    
    // Debug
    static void printMappings();
};
