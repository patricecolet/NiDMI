#include "ComponentRegistry.h"
#include <cstring>

// Initialisation des membres statiques
std::vector<ComponentDefinition> ComponentRegistry::definitions_;
bool ComponentRegistry::initialized_ = false;

void ComponentRegistry::init() {
    if (initialized_) return;
    
    // Initialiser le ValidationRegistry d'abord
    ValidationRegistry::init();
    
    // Enregistrer tous les composants disponibles
    
    // === COMPOSANTS D'ENTRÉE ===
    
    // Potentiomètre - composant simple, 1 pin analogique
    {
        ComponentDefinition def = {};
        def.id = "potentiometer";
        def.displayName = "Potentiomètre";
        def.icon = nullptr;
        def.type = ComponentType::POTENTIOMETER;
        def.pinType = PinType::PIN_ANALOG;
        def.implemented = true;
        def.isComplex = false;
        def.additionalPinCount = 0;  // Pas de pins additionnelles
        definitions_.push_back(def);
    }
    
    // Bouton - composant simple, 1 pin digitale
    {
        ComponentDefinition def = {};
        def.id = "button";
        def.displayName = "Bouton";
        def.icon = nullptr;
        def.type = ComponentType::BUTTON;
        def.pinType = PinType::PIN_DIGITAL;
        def.implemented = true;
        def.isComplex = false;
        def.additionalPinCount = 0;
        definitions_.push_back(def);
    }
    
    // MUX - composant complexe, pin analogique (SIG) + 4 pins digitales (S0-S3) + 1 optionnelle (EN)
    {
        ComponentDefinition def = {};
        def.id = "mux";
        def.displayName = "Multiplexeur";
        def.icon = nullptr;
        def.type = ComponentType::MUX;
        def.pinType = PinType::PIN_ANALOG;  // Pin principale (SIG)
        def.implemented = true;
        def.isComplex = true;
        def.additionalPinCount = 5;  // S0, S1, S2, S3, EN
        
        // Pins d'adresse (obligatoires)
        def.additionalPins[0] = {"s0", "S0", PinType::PIN_DIGITAL, false, 255};
        def.additionalPins[1] = {"s1", "S1", PinType::PIN_DIGITAL, false, 255};
        def.additionalPins[2] = {"s2", "S2", PinType::PIN_DIGITAL, false, 255};
        def.additionalPins[3] = {"s3", "S3", PinType::PIN_DIGITAL, false, 255};
        // Pin Enable (optionnelle)
        def.additionalPins[4] = {"en", "Enable", PinType::PIN_DIGITAL, true, 255};
        
        definitions_.push_back(def);
    }
    
    // === COMPOSANTS DE SORTIE ===
    
    // LED - composant simple, 1 pin PWM
    {
        ComponentDefinition def = {};
        def.id = "led";
        def.displayName = "LED";
        def.icon = nullptr;
        def.type = ComponentType::LED;
        def.pinType = PinType::PIN_PWM;
        def.implemented = true;
        def.isComplex = false;
        def.additionalPinCount = 0;
        definitions_.push_back(def);
    }
    
    // === BUS (composants spéciaux avec pins fixes) ===
    
    // I2C - 2 pins fixes (SDA, SCL)
    {
        ComponentDefinition def = {};
        def.id = "i2c";
        def.displayName = "I2C";
        def.icon = nullptr;
        def.type = ComponentType::BUTTON;  // Type générique pour l'instant
        def.pinType = PinType::PIN_DIGITAL;
        def.implemented = true;
        def.isComplex = true;  // Complexe car multi-pin
        def.additionalPinCount = 1;  // SCL est la pin additionnelle
        def.additionalPins[0] = {"scl", "SCL", PinType::PIN_DIGITAL, false, 255};
        definitions_.push_back(def);
    }
    
    // SPI - 3 pins fixes (MOSI, MISO, SCK)
    {
        ComponentDefinition def = {};
        def.id = "spi";
        def.displayName = "SPI";
        def.icon = nullptr;
        def.type = ComponentType::BUTTON;  // Type générique pour l'instant
        def.pinType = PinType::PIN_DIGITAL;
        def.implemented = true;
        def.isComplex = true;
        def.additionalPinCount = 2;  // MISO et SCK sont les pins additionnelles
        def.additionalPins[0] = {"miso", "MISO", PinType::PIN_DIGITAL, false, 255};
        def.additionalPins[1] = {"sck", "SCK", PinType::PIN_DIGITAL, false, 255};
        definitions_.push_back(def);
    }
    
    // UART - 2 pins fixes (TX, RX)
    {
        ComponentDefinition def = {};
        def.id = "uart";
        def.displayName = "UART";
        def.icon = nullptr;
        def.type = ComponentType::BUTTON;  // Type générique pour l'instant
        def.pinType = PinType::PIN_DIGITAL;
        def.implemented = true;
        def.isComplex = true;
        def.additionalPinCount = 1;  // RX est la pin additionnelle
        def.additionalPins[0] = {"rx", "RX", PinType::PIN_DIGITAL, false, 255};
        definitions_.push_back(def);
    }
    
    initialized_ = true;
}

const std::vector<ComponentDefinition>& ComponentRegistry::getAll() {
    return definitions_;
}

const ComponentDefinition* ComponentRegistry::findById(const char* id) {
    for (const auto& def : definitions_) {
        if (strcmp(def.id, id) == 0) {
            return &def;
        }
    }
    return nullptr;
}

const ComponentDefinition* ComponentRegistry::findByType(ComponentType type) {
    for (const auto& def : definitions_) {
        if (def.type == type) {
            return &def;
        }
    }
    return nullptr;
}

int ComponentRegistry::toJsonArray(char* buffer, size_t bufferSize) {
    if (bufferSize < 3) return 0;
    
    int written = 0;
    buffer[written++] = '[';
    
    bool first = true;
    for (const auto& def : definitions_) {
        if (!first) {
            if (written >= (int)bufferSize - 1) break;
            buffer[written++] = ',';
        }
        first = false;
        
        // Calculer l'espace restant
        size_t remaining = bufferSize - written - 1; // -1 pour le ']' final
        
        // Écrire la définition en JSON
        int defLen = def.toJson(buffer + written, remaining);
        if (defLen <= 0 || defLen >= (int)remaining) break;
        
        written += defLen;
    }
    
    buffer[written++] = ']';
    buffer[written] = '\0';
    
    return written;
}

size_t ComponentRegistry::count() {
    return definitions_.size();
}
