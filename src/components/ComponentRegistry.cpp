#include "ComponentRegistry.h"
#include "basic/PotentiometerDef.h"
#include "basic/ButtonDef.h"
#include "basic/LedDef.h"
#include "multiplexer/MuxDef.h"
#include <cstring>
#include <utility>  // pour std::move

// Initialisation des membres statiques
// Utiliser un pointeur pour éviter l'initialisation statique avant setup() (crash sur ESP32-C3)
std::vector<ComponentDefinition>* ComponentRegistry::definitions_ptr = nullptr;
bool ComponentRegistry::initialized_ = false;

void ComponentRegistry::init() {
    if (initialized_) return;
    
    // Créer le vector seulement maintenant (heap est prêt après setup())
    // Cela évite les problèmes d'initialisation statique sur ESP32-C3
    if (definitions_ptr == nullptr) {
        definitions_ptr = new (std::nothrow) std::vector<ComponentDefinition>();
        if (definitions_ptr == nullptr) {
            // En mode production, Serial n'est peut-être pas encore initialisé
            // mais on essaie quand même pour le debug
            #ifdef ARDUINO
            Serial.printf("[ComponentRegistry] CRITICAL: Failed to allocate vector\n");
            #endif
            return;
        }
        // Réserver l'espace dans le vector pour éviter les réallocations
        // qui peuvent créer des copies temporaires
        definitions_ptr->reserve(6);  // Augmenté à 6 pour permettre l'ajout futur
    }
    
    // Initialiser le ValidationRegistry d'abord
    ValidationRegistry::init();
    
    // === FAMILLE BASIC ===
    // Composants simples : Potentiomètre, Bouton, LED
    // Créer chaque définition dans un bloc séparé pour limiter la portée sur la pile
    
    {
        ComponentDefinition def = Components::Potentiometer::createDefinition();
        if (def.id != nullptr) {
            definitions_ptr->push_back(std::move(def));
        }
    }
    
    {
        ComponentDefinition def = Components::Button::createDefinition();
        if (def.id != nullptr) {
            definitions_ptr->push_back(std::move(def));
        }
    }
    
    {
        ComponentDefinition def = Components::Led::createDefinition();
        if (def.id != nullptr) {
            definitions_ptr->push_back(std::move(def));
        }
    }
    
    // === FAMILLE MULTIPLEXER ===
    // Multiplexeurs analogiques : HC4067, HC4051, etc.
    
    {
        ComponentDefinition def = Components::HC4067::createDefinition();
        if (def.id != nullptr) {
            definitions_ptr->push_back(std::move(def));
        }
    }
    
    {
        ComponentDefinition def = Components::HC4051::createDefinition();
        if (def.id != nullptr) {
            definitions_ptr->push_back(std::move(def));
        }
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
    // Créer un vector vide si pas encore initialisé (éviter crash)
    static std::vector<ComponentDefinition> empty_vector;
    if (definitions_ptr == nullptr) {
        return empty_vector;
    }
    return *definitions_ptr;
}

const ComponentDefinition* ComponentRegistry::findById(const char* id) {
    if (definitions_ptr == nullptr) return nullptr;
    for (const auto& def : *definitions_ptr) {
        if (strcmp(def.id, id) == 0) {
            return &def;
        }
    }
    return nullptr;
}

const ComponentDefinition* ComponentRegistry::findByType(ComponentType type) {
    if (definitions_ptr == nullptr) return nullptr;
    for (const auto& def : *definitions_ptr) {
        if (def.type == type) {
            return &def;
        }
    }
    return nullptr;
}

int ComponentRegistry::toJsonArray(char* buffer, size_t bufferSize) {
    if (bufferSize < 3 || definitions_ptr == nullptr) return 0;
    
    int written = 0;
    buffer[written++] = '[';
    
    bool first = true;
    for (const auto& def : *definitions_ptr) {
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
    if (definitions_ptr == nullptr) return 0;
    return definitions_ptr->size();
}

void ComponentRegistry::cleanup() {
    if (definitions_ptr != nullptr) {
        for (auto& def : *definitions_ptr) {
            def.cleanup();
        }
        definitions_ptr->clear();
        delete definitions_ptr;
        definitions_ptr = nullptr;
    }
    initialized_ = false;
}
