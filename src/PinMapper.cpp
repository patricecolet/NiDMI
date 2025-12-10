#include "PinMapper.h"

// Variables statiques
McuType PinMapper::detected_mcu = McuType::UNKNOWN;
bool PinMapper::mcu_detected = false;

// ============================================================================
// ESP32-C3 (XIAO-ESP32C3) - Pins physiques
// ============================================================================
const PhysicalPin PinMapper::c3_physical_pins[] = {
    {2,  true,  true,  false, "D0"},  // GPIO2: ADC, PWM
    {3,  true,  true,  false, "D1"},  // GPIO3: ADC, PWM
    {4,  true,  true,  false, "D2"},  // GPIO4: ADC, PWM
    {5,  true,  true,  false, "D3"},  // GPIO5: ADC, PWM
    {6,  false, true,  false, "D4"},  // GPIO6: PWM (I2C SDA)
    {7,  false, true,  false, "D5"},  // GPIO7: PWM (I2C SCL)
    {21, false, true,  false, "D6"},  // GPIO21: PWM (UART TX)
    {20, false, true,  false, "D7"},  // GPIO20: PWM (UART RX)
    {8,  false, true,  false, "D8"},  // GPIO8: PWM (SPI SCK)
    {9,  false, true,  false, "D9"},  // GPIO9: PWM (SPI MISO)
    {10, false, true,  false, "D10"}  // GPIO10: PWM (SPI MOSI)
};

const size_t PinMapper::c3_physical_pin_count = sizeof(c3_physical_pins) / sizeof(c3_physical_pins[0]);

// Aliases ESP32-C3
const PinAlias PinMapper::c3_aliases[] = {
    // Aliases analogiques
    {"A0", 2},   // D0 = A0
    {"A1", 3},   // D1 = A1
    {"A2", 4},   // D2 = A2
    // A3 n'existe pas sur ESP32-C3
    
    // Aliases bus I2C
    {"SDA", 6},  // D4 = SDA
    {"SCL", 7},  // D5 = SCL
    
    // Aliases bus SPI
    {"MOSI", 10}, // D10 = MOSI
    {"MISO", 9},  // D9 = MISO
    {"SCK", 8},   // D8 = SCK
    
    // Aliases bus UART
    {"TX", 21},   // D6 = TX
    {"RX", 20}    // D7 = RX
};

const size_t PinMapper::c3_alias_count = sizeof(c3_aliases) / sizeof(c3_aliases[0]);

// ============================================================================
// ESP32-S3 (XIAO-ESP32S3) - Pins physiques
// ============================================================================
const PhysicalPin PinMapper::s3_physical_pins[] = {
    {1,  true,  true,  true,  "D0"},  // GPIO1: ADC, PWM, Touch
    {2,  true,  true,  true,  "D1"},  // GPIO2: ADC, PWM, Touch
    {3,  true,  true,  true,  "D2"},  // GPIO3: ADC, PWM, Touch
    {4,  true,  true,  true,  "D3"},  // GPIO4: ADC, PWM, Touch
    {5,  true,  true,  true,  "D4"},  // GPIO5: ADC, PWM, Touch
    {6,  true,  true,  true,  "D5"},  // GPIO6: ADC, PWM, Touch
    {7,  true,  true,  true,  "D8"},  // GPIO7: ADC, PWM, Touch (SPI SCK)
    {8,  true,  true,  true,  "D9"},  // GPIO8: ADC, PWM, Touch (SPI MISO)
    {9,  true,  true,  true,  "D10"}, // GPIO9: ADC, PWM, Touch (SPI MOSI)
    {43, false, true,  false, "D6"},  // GPIO43: PWM (UART TX) - Pas ADC/Touch
    {44, false, true,  false, "D7"}   // GPIO44: PWM (UART RX) - Pas ADC/Touch
};

const size_t PinMapper::s3_physical_pin_count = sizeof(s3_physical_pins) / sizeof(s3_physical_pins[0]);

// Aliases ESP32-S3
// Note: Les touch pins (T1-T9) ne sont pas des aliases séparés,
// touch est une fonction disponible sur les pins ADC (GPIO1-9)
const PinAlias PinMapper::s3_aliases[] = {
    // Aliases analogiques
    {"A0", 1},   // D0 = A0
    {"A1", 2},   // D1 = A1
    {"A2", 3},   // D2 = A2
    {"A3", 4},   // D3 = A3
    {"A4", 5},   // D4 = A4
    {"A5", 6},   // D5 = A5
    {"A8", 7},   // D8 = A8
    {"A9", 8},   // D9 = A9
    {"A10", 9},  // D10 = A10
    
    // Aliases bus I2C
    {"SDA", 5},  // D4 = SDA (GPIO5)
    {"SCL", 6},  // D5 = SCL (GPIO6)
    
    // Aliases bus SPI
    {"MOSI", 9}, // D10 = MOSI (GPIO9)
    {"MISO", 8}, // D9 = MISO (GPIO8)
    {"SCK", 7},  // D8 = SCK (GPIO7)
    
    // Aliases bus UART
    {"TX", 43},  // D6 = TX (GPIO43)
    {"RX", 44}   // D7 = RX (GPIO44)
};

const size_t PinMapper::s3_alias_count = sizeof(s3_aliases) / sizeof(s3_aliases[0]);

// ============================================================================
// Fonctions internes
// ============================================================================

/**
 * @brief Trouve une pin physique par son GPIO
 */
const PhysicalPin* PinMapper::findPhysicalPin(uint8_t gpio) {
    detectMcu();
    
    const PhysicalPin* pins;
    size_t count;
    
    switch (detected_mcu) {
        case McuType::ESP32_C3:
            pins = c3_physical_pins;
            count = c3_physical_pin_count;
            break;
        case McuType::ESP32_S3:
            pins = s3_physical_pins;
            count = s3_physical_pin_count;
            break;
        default:
            return nullptr;
    }
    
    for (size_t i = 0; i < count; i++) {
        if (pins[i].gpio == gpio) {
            return &pins[i];
        }
    }
    
    return nullptr;
}

/**
 * @brief Résout un alias vers un GPIO
 * Retourne 255 si l'alias n'existe pas
 */
uint8_t PinMapper::resolveAlias(const char* label) {
    detectMcu();
    
    const PinAlias* aliases;
    size_t count;
    
    switch (detected_mcu) {
        case McuType::ESP32_C3:
            aliases = c3_aliases;
            count = c3_alias_count;
            break;
        case McuType::ESP32_S3:
            aliases = s3_aliases;
            count = s3_alias_count;
            break;
        default:
            return 255;
    }
    
    for (size_t i = 0; i < count; i++) {
        if (strcmp(aliases[i].alias, label) == 0) {
            return aliases[i].gpio;
        }
    }
    
    return 255; // Alias non trouvé
}

// ============================================================================
// Détection MCU
// ============================================================================

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
        // Fallback : test des pins (nécessaire car les macros ne fonctionnent pas dans Arduino IDE)
        Serial.println("[PinMapper] Aucune macro détectée, test des pins GPIO21/GPIO43...");
        
        pinMode(21, INPUT);
        pinMode(43, INPUT);
        delay(10); // Laisser le temps à la pin de s'initialiser
        
        bool gpio21_exists = (digitalRead(21) == 0 || digitalRead(21) == 1);
        bool gpio43_exists = (digitalRead(43) == 0 || digitalRead(43) == 1);
        
        if (gpio21_exists && !gpio43_exists) {
            detected_mcu = McuType::ESP32_C3;
            Serial.println("[PinMapper] MCU détecté: ESP32-C3 (via test pins GPIO21/GPIO43)");
        } else if (gpio43_exists) { // Si GPIO43 existe, c'est un S3 (même si GPIO21 existe aussi)
            detected_mcu = McuType::ESP32_S3;
            Serial.println("[PinMapper] MCU détecté: ESP32-S3 (via test pins GPIO21/GPIO43)");
        } else {
            Serial.println("[PinMapper] MCU non détecté via macros ou pins, par défaut ESP32-C3.");
            detected_mcu = McuType::ESP32_C3; // Fallback par défaut
        }
    #endif
    
    mcu_detected = true;
    Serial.printf("[PinMapper] MCU final: %d\n", (int)detected_mcu);
    return detected_mcu;
}

// ============================================================================
// API publique
// ============================================================================

uint8_t PinMapper::labelToGpio(const String& label) {
    return labelToGpio(label.c_str());
}

uint8_t PinMapper::labelToGpio(const char* label) {
    detectMcu();
    
    // 1. Vérifier si c'est un alias
    uint8_t gpio_from_alias = resolveAlias(label);
    if (gpio_from_alias != 255) {
        return gpio_from_alias;
    }
    
    // 2. Vérifier si c'est une pin physique (par label principal)
    const PhysicalPin* pins;
    size_t count;
    
    switch (detected_mcu) {
        case McuType::ESP32_C3:
            pins = c3_physical_pins;
            count = c3_physical_pin_count;
            break;
        case McuType::ESP32_S3:
            pins = s3_physical_pins;
            count = s3_physical_pin_count;
            break;
        default:
            return 255;
    }
    
    for (size_t i = 0; i < count; i++) {
        if (strcmp(pins[i].primary_label, label) == 0) {
            return pins[i].gpio;
        }
    }
    
    return 255; // Label non trouvé
}

String PinMapper::gpioToLabel(uint8_t gpio) {
    const PhysicalPin* pin = findPhysicalPin(gpio);
    if (pin) {
        return String(pin->primary_label);
    }
    return "";
}

bool PinMapper::hasAdc(uint8_t gpio) {
    const PhysicalPin* pin = findPhysicalPin(gpio);
    return pin ? pin->has_adc : false;
}

bool PinMapper::hasPwm(uint8_t gpio) {
    const PhysicalPin* pin = findPhysicalPin(gpio);
    return pin ? pin->has_pwm : false;
}

bool PinMapper::hasTouch(uint8_t gpio) {
    const PhysicalPin* pin = findPhysicalPin(gpio);
    return pin ? pin->has_touch : false;
}

String PinMapper::getMcuName() {
    switch (detected_mcu) {
        case McuType::ESP32_C3: return "ESP32-C3";
        case McuType::ESP32_S3: return "ESP32-S3";
        default: return "Unknown";
    }
}

// ============================================================================
// API de compatibilité (génère PinMapping depuis PhysicalPin + Aliases)
// ============================================================================

// Buffer statique pour stocker les PinMapping générés
static PinMapping* generated_mappings = nullptr;
static size_t generated_mapping_count = 0;

const PinMapping* PinMapper::getAllMappings() {
    detectMcu();
    
    // Libérer l'ancien buffer si nécessaire
    if (generated_mappings != nullptr) {
        delete[] generated_mappings;
        generated_mappings = nullptr;
        generated_mapping_count = 0;
    }
    
    const PhysicalPin* pins;
    size_t pin_count;
    const PinAlias* aliases;
    size_t alias_count;
    
    switch (detected_mcu) {
        case McuType::ESP32_C3:
            pins = c3_physical_pins;
            pin_count = c3_physical_pin_count;
            aliases = c3_aliases;
            alias_count = c3_alias_count;
            break;
        case McuType::ESP32_S3:
            pins = s3_physical_pins;
            pin_count = s3_physical_pin_count;
            aliases = s3_aliases;
            alias_count = s3_alias_count;
            break;
        default:
            return nullptr;
    }
    
    // Allouer le buffer : pins physiques + aliases
    generated_mapping_count = pin_count + alias_count;
    generated_mappings = new PinMapping[generated_mapping_count];
    
    // Copier les pins physiques
    for (size_t i = 0; i < pin_count; i++) {
        generated_mappings[i].label = pins[i].primary_label;
        generated_mappings[i].gpio = pins[i].gpio;
        generated_mappings[i].has_adc = pins[i].has_adc;
        generated_mappings[i].has_pwm = pins[i].has_pwm;
        generated_mappings[i].has_touch = pins[i].has_touch;
    }
    
    // Ajouter les aliases
    for (size_t i = 0; i < alias_count; i++) {
        const PhysicalPin* pin = findPhysicalPin(aliases[i].gpio);
        if (pin) {
            generated_mappings[pin_count + i].label = aliases[i].alias;
            generated_mappings[pin_count + i].gpio = pin->gpio;
            generated_mappings[pin_count + i].has_adc = pin->has_adc;
            generated_mappings[pin_count + i].has_pwm = pin->has_pwm;
            generated_mappings[pin_count + i].has_touch = pin->has_touch;
        }
    }
    
    return generated_mappings;
}

size_t PinMapper::getMappingCount() {
    detectMcu();
    
    switch (detected_mcu) {
        case McuType::ESP32_C3:
            return c3_physical_pin_count + c3_alias_count;
        case McuType::ESP32_S3:
            return s3_physical_pin_count + s3_alias_count;
        default:
            return 0;
    }
}

void PinMapper::printMappings() {
    detectMcu();
    Serial.printf("[PinMapper] MCU: %s\n", getMcuName().c_str());
    
    const PinMapping* mappings = getAllMappings();
    size_t count = getMappingCount();
    
    Serial.println("[PinMapper] Pins physiques:");
    const PhysicalPin* pins;
    size_t pin_count;
    
    switch (detected_mcu) {
        case McuType::ESP32_C3:
            pins = c3_physical_pins;
            pin_count = c3_physical_pin_count;
            break;
        case McuType::ESP32_S3:
            pins = s3_physical_pins;
            pin_count = s3_physical_pin_count;
            break;
        default:
            return;
    }
    
    for (size_t i = 0; i < pin_count; i++) {
        Serial.printf("  %s → GPIO%d (ADC:%s PWM:%s Touch:%s)\n",
            pins[i].primary_label,
            pins[i].gpio,
            pins[i].has_adc ? "✓" : "✗",
            pins[i].has_pwm ? "✓" : "✗",
            pins[i].has_touch ? "✓" : "✗"
        );
    }
    
    Serial.println("[PinMapper] Tous les mappings (physiques + aliases):");
    for (size_t i = 0; i < count; i++) {
        Serial.printf("  %s → GPIO%d (ADC:%s PWM:%s Touch:%s)\n",
            mappings[i].label,
            mappings[i].gpio,
            mappings[i].has_adc ? "✓" : "✗",
            mappings[i].has_pwm ? "✓" : "✗",
            mappings[i].has_touch ? "✓" : "✗"
        );
    }
}
