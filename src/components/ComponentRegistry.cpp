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
    // Format: id, displayName, icon, type, pinType, implemented, isComplex
    
    // === COMPOSANTS D'ENTRÉE ===
    
    // Potentiomètre - composant simple, pin analogique
    definitions_.push_back({
        "potentiometer",
        "Potentiomètre",
        nullptr,  // pas d'icône pour l'instant
        ComponentType::POTENTIOMETER,
        PinType::PIN_ANALOG,
        true,     // implémenté
        false     // simple
    });
    
    // Bouton - composant simple, pin digitale
    definitions_.push_back({
        "button",
        "Bouton",
        nullptr,
        ComponentType::BUTTON,
        PinType::PIN_DIGITAL,
        true,     // implémenté
        false     // simple
    });
    
    // MUX - composant complexe, pin analogique (SIG)
    definitions_.push_back({
        "mux",
        "Multiplexeur",
        nullptr,
        ComponentType::MUX,
        PinType::PIN_ANALOG,
        true,     // implémenté
        true      // complexe (nécessite MuxManager)
    });
    
    // === COMPOSANTS DE SORTIE ===
    
    // LED - composant simple, pin PWM
    definitions_.push_back({
        "led",
        "LED",
        nullptr,
        ComponentType::LED,
        PinType::PIN_PWM,
        true,     // implémenté
        false     // simple
    });
    
    // === COMPOSANTS FUTURS (non implémentés, grisés dans l'UI) ===
    
    // Exemple: Encoder rotatif - à implémenter plus tard
    // definitions_.push_back({
    //     "encoder",
    //     "Encodeur",
    //     nullptr,
    //     ComponentType::ENCODER,  // à ajouter dans ComponentType enum
    //     PinType::DIGITAL,
    //     false,    // pas encore implémenté
    //     false     // simple
    // });
    
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
