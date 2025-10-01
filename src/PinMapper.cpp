#include "PinMapper.h"

// Variables statiques
McuType PinMapper::detected_mcu = McuType::UNKNOWN;
bool PinMapper::mcu_detected = false;

// Mappings ESP32-C3 (XIAO-ESP32C3)
const PinMapping PinMapper::c3_mappings[] = {
    {"D0", 2,  true,  true,  false},
    {"D1", 3,  true,  true,  false},
    {"D2", 4,  true,  true,  false},
    {"D3", 5,  true,  true,  false},
    {"D4", 6,  false, true,  false},
    {"D5", 7,  false, true,  false},
    {"D6", 21, false, true,  false},
    {"D7", 20, false, true,  false},
    {"D8", 8,  false, true,  false},
    {"D9", 9,  false, true,  false},
    {"D10", 10, false, true,  false},
    {"A0", 2,  true,  true,  false},  // D0 = A0
    {"A1", 3,  true,  true,  false},  // D1 = A1
    {"A2", 4,  true,  true,  false},  // D2 = A2
    {"A3", 5,  true,  true,  false},  // D3 = A3
    {"SDA", 6, false, true,  false},  // I2C SDA
    {"SCL", 7, false, true,  false},  // I2C SCL
    {"MOSI", 10, false, true,  false}, // SPI MOSI
    {"MISO", 9, false, true,  false}, // SPI MISO
    {"SCK", 8, false, true,  false},  // SPI SCK
    {"TX", 21, false, true,  false},  // UART TX
    {"RX", 20, false, true,  false}   // UART RX
};

const size_t PinMapper::c3_mapping_count = sizeof(c3_mappings) / sizeof(c3_mappings[0]);

// Mappings ESP32-S3 (XIAO-ESP32S3)
const PinMapping PinMapper::s3_mappings[] = {
    {"D0", 1,  true,  true,  true},
    {"D1", 2,  true,  true,  true},
    {"D2", 3,  true,  true,  true},
    {"D3", 4,  true,  true,  true},
    {"D4", 5,  true,  true,  true},
    {"D5", 6,  false, true,  true},
    {"D6", 7,  false, true,  true},
    {"D7", 8,  false, true,  true},
    {"D8", 9,  false, true,  true},
    {"D9", 10, false, true,  true},
    {"A0", 1,  true,  true,  true},   // D0 = A0
    {"A1", 2,  true,  true,  true},   // D1 = A1
    {"A2", 3,  true,  true,  true},   // D2 = A2
    {"A3", 4,  true,  true,  true},   // D3 = A3
    {"A4", 5,  true,  true,  true},   // D4 = A4
    {"SDA", 4, true,  true,  true},   // I2C SDA
    {"SCL", 5, true,  true,  true},   // I2C SCL
    {"MOSI", 7, false, true,  true},  // SPI MOSI
    {"MISO", 6, false, true,  true},  // SPI MISO
    {"SCK", 8, false, true,  true},   // SPI SCK
    {"TX", 43, false, true,  false},  // UART TX
    {"RX", 44, false, true,  false}   // UART RX
};

const size_t PinMapper::s3_mapping_count = sizeof(s3_mappings) / sizeof(s3_mappings[0]);

McuType PinMapper::detectMcu() {
    if (mcu_detected) return detected_mcu;
    
    // Détection basée sur les macros de compilation
    #ifdef CONFIG_IDF_TARGET_ESP32C3
        detected_mcu = McuType::ESP32_C3;
        Serial.println("[PinMapper] MCU détecté: ESP32-C3 (via CONFIG_IDF_TARGET_ESP32C3)");
    #elif defined(CONFIG_IDF_TARGET_ESP32S3)
        detected_mcu = McuType::ESP32_S3;
        Serial.println("[PinMapper] MCU détecté: ESP32-S3 (via CONFIG_IDF_TARGET_ESP32S3)");
    #elif defined(ARDUINO_ESP32C3_DEV) || defined(ARDUINO_ESP32C3)
        detected_mcu = McuType::ESP32_C3;
        Serial.println("[PinMapper] MCU détecté: ESP32-C3 (via ARDUINO_ESP32C3)");
    #elif defined(ARDUINO_ESP32S3_DEV) || defined(ARDUINO_ESP32S3)
        detected_mcu = McuType::ESP32_S3;
        Serial.println("[PinMapper] MCU détecté: ESP32-S3 (via ARDUINO_ESP32S3)");
    #else
        Serial.println("[PinMapper] Aucune macro détectée, test des pins...");
        pinMode(21, INPUT);
        pinMode(43, INPUT);
        
        // Test simple: si GPIO21 fonctionne mais pas GPIO43 → C3
        // Si GPIO43 fonctionne mais pas GPIO21 → S3
        if (digitalRead(21) != digitalRead(43)) {
            detected_mcu = McuType::ESP32_C3; // GPIO21 existe
            Serial.println("[PinMapper] MCU détecté: ESP32-C3 (via test pins)");
        } else {
            detected_mcu = McuType::ESP32_S3; // GPIO43 existe
            Serial.println("[PinMapper] MCU détecté: ESP32-S3 (via test pins)");
        }
    #endif
    
    mcu_detected = true;
    Serial.printf("[PinMapper] MCU final: %d\n", (int)detected_mcu);
    return detected_mcu;
}

uint8_t PinMapper::labelToGpio(const String& label) {
    return labelToGpio(label.c_str());
}

uint8_t PinMapper::labelToGpio(const char* label) {
    detectMcu();
    
    const PinMapping* mappings;
    size_t count;
    
    switch (detected_mcu) {
        case McuType::ESP32_C3:
            mappings = c3_mappings;
            count = c3_mapping_count;
            break;
        case McuType::ESP32_S3:
            mappings = s3_mappings;
            count = s3_mapping_count;
            break;
        default:
            return 255; // Invalid
    }
    
    for (size_t i = 0; i < count; i++) {
        if (strcmp(mappings[i].label, label) == 0) {
            return mappings[i].gpio;
        }
    }
    
    return 255; // Not found
}

String PinMapper::gpioToLabel(uint8_t gpio) {
    detectMcu();
    
    const PinMapping* mappings;
    size_t count;
    
    switch (detected_mcu) {
        case McuType::ESP32_C3:
            mappings = c3_mappings;
            count = c3_mapping_count;
            break;
        case McuType::ESP32_S3:
            mappings = s3_mappings;
            count = s3_mapping_count;
            break;
        default:
            return "";
    }
    
    for (size_t i = 0; i < count; i++) {
        if (mappings[i].gpio == gpio) {
            return String(mappings[i].label);
        }
    }
    
    return "";
}

bool PinMapper::hasAdc(uint8_t gpio) {
    detectMcu();
    
    const PinMapping* mappings;
    size_t count;
    
    switch (detected_mcu) {
        case McuType::ESP32_C3:
            mappings = c3_mappings;
            count = c3_mapping_count;
            break;
        case McuType::ESP32_S3:
            mappings = s3_mappings;
            count = s3_mapping_count;
            break;
        default:
            return false;
    }
    
    for (size_t i = 0; i < count; i++) {
        if (mappings[i].gpio == gpio) {
            return mappings[i].has_adc;
        }
    }
    
    return false;
}

bool PinMapper::hasPwm(uint8_t gpio) {
    detectMcu();
    
    const PinMapping* mappings;
    size_t count;
    
    switch (detected_mcu) {
        case McuType::ESP32_C3:
            mappings = c3_mappings;
            count = c3_mapping_count;
            break;
        case McuType::ESP32_S3:
            mappings = s3_mappings;
            count = s3_mapping_count;
            break;
        default:
            return false;
    }
    
    for (size_t i = 0; i < count; i++) {
        if (mappings[i].gpio == gpio) {
            return mappings[i].has_pwm;
        }
    }
    
    return false;
}

bool PinMapper::hasTouch(uint8_t gpio) {
    detectMcu();
    
    const PinMapping* mappings;
    size_t count;
    
    switch (detected_mcu) {
        case McuType::ESP32_C3:
            mappings = c3_mappings;
            count = c3_mapping_count;
            break;
        case McuType::ESP32_S3:
            mappings = s3_mappings;
            count = s3_mapping_count;
            break;
        default:
            return false;
    }
    
    for (size_t i = 0; i < count; i++) {
        if (mappings[i].gpio == gpio) {
            return mappings[i].has_touch;
        }
    }
    
    return false;
}

String PinMapper::getMcuName() {
    switch (detected_mcu) {
        case McuType::ESP32_C3: return "ESP32-C3";
        case McuType::ESP32_S3: return "ESP32-S3";
        default: return "Unknown";
    }
}

const PinMapping* PinMapper::getAllMappings() {
    detectMcu();
    
    switch (detected_mcu) {
        case McuType::ESP32_C3: return c3_mappings;
        case McuType::ESP32_S3: return s3_mappings;
        default: return nullptr;
    }
}

size_t PinMapper::getMappingCount() {
    detectMcu();
    
    switch (detected_mcu) {
        case McuType::ESP32_C3: return c3_mapping_count;
        case McuType::ESP32_S3: return s3_mapping_count;
        default: return 0;
    }
}

void PinMapper::printMappings() {
    detectMcu();
    Serial.println("[PinMapper] MCU: ");
    Serial.printf("[PinMapper] MCU: %s\n", getMcuName());
    
    const PinMapping* mappings = getAllMappings();
    size_t count = getMappingCount();
    
    for (size_t i = 0; i < count; i++) {
        Serial.printf( "  %s → GPIO%d ADC:%s PWM:%s Touch:%s)\n",
            mappings[i].label,
            mappings[i].gpio,
            mappings[i].has_adc ? "✓" : "✗",
            mappings[i].has_pwm ? "✓" : "✗",
            mappings[i].has_touch ? "✓" : "✗"
        );
    }
}
