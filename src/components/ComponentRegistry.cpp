#include "ComponentRegistry.h"
#include "basic/PotentiometerDef.h"
#include "basic/ButtonDef.h"
#include "basic/LedDef.h"
#include "multiplexer/MuxDef.h"
#include <cstring>
#include <utility>  // pour std::move

// Initialisation des membres statiques
std::vector<ComponentDefinition> ComponentRegistry::definitions_;
bool ComponentRegistry::initialized_ = false;

void ComponentRegistry::init() {
    if (initialized_) return;
    
    // Initialiser le ValidationRegistry d'abord
    ValidationRegistry::init();
    
    // Réserver l'espace dans le vector pour éviter les réallocations
    // qui peuvent créer des copies temporaires
    definitions_.reserve(5);
    
    // === FAMILLE BASIC ===
    // Composants simples : Potentiomètre, Bouton, LED
    // Créer chaque définition dans un bloc séparé pour limiter la portée sur la pile
    
    {
        ComponentDefinition def = Components::Potentiometer::createDefinition();
        definitions_.push_back(std::move(def));
    }
    
    {
        ComponentDefinition def = Components::Button::createDefinition();
        definitions_.push_back(std::move(def));
    }
    
    {
        ComponentDefinition def = Components::Led::createDefinition();
        definitions_.push_back(std::move(def));
    }
    
    // === FAMILLE MULTIPLEXER ===
    // Multiplexeurs analogiques : HC4067, HC4051, etc.
    
    {
        ComponentDefinition def = Components::HC4067::createDefinition();
        definitions_.push_back(std::move(def));
    }
    
    {
        ComponentDefinition def = Components::HC4051::createDefinition();
        definitions_.push_back(std::move(def));
    }
    
    // === FAMILLES FUTURES ===
    // ENCODER : encodeurs rotatifs
    // DISPLAY : écrans OLED, LCD
    
    // Note: I2C et SPI ne sont PAS des composants, ce sont des bus hardware.
    // Les pins I2C/SPI sont identifiées via caps.bus dans l'API /api/pins/caps.
    // Les composants qui utilisent ces bus (écran OLED, DAC, etc.) seront ajoutés plus tard.
    
    // Note: TX et RX (UART) sont des pins digitales indépendantes.
    // Elles peuvent être utilisées séparément (ex: TX seul pour MIDI OUT).
    
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

void ComponentRegistry::cleanup() {
    for (auto& def : definitions_) {
        def.cleanup();
    }
    definitions_.clear();
    initialized_ = false;
}
