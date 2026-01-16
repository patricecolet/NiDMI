#include "ValidationRegistry.h"
#include "../utils/PinMapper.h"
#include "../managers/MuxValidator.h"
#include "../managers/MuxManager.h"

// Initialisation du map statique
std::map<std::string, ValidationRegistry::ValidatorFunc> ValidationRegistry::validators_;

void ValidationRegistry::registerValidator(const char* componentId, ValidatorFunc validator) {
    validators_[componentId] = validator;
}

bool ValidationRegistry::validate(const char* componentId, uint8_t gpio, const void* config) {
    auto it = validators_.find(componentId);
    if (it == validators_.end()) {
        // Pas de validator enregistré = invalide par défaut
        return false;
    }
    return it->second(gpio, config);
}

bool ValidationRegistry::hasValidator(const char* componentId) {
    return validators_.find(componentId) != validators_.end();
}

void ValidationRegistry::init() {
    // Potentiomètre : nécessite une pin ADC
    registerValidator("potentiometer", [](uint8_t gpio, const void*) {
        return PinMapper::hasAdc(gpio);
    });
    
    // Bouton : n'importe quelle pin GPIO valide (on vérifie juste que c'est dans la plage)
    registerValidator("button", [](uint8_t gpio, const void*) {
        // Un GPIO est valide si il est dans la plage des pins du MCU
        return gpio < 48; // ESP32 a max 48 GPIOs
    });
    
    // LED : pin digitale (PWM optionnel, utilisé si disponible et ledMode="pwm")
    registerValidator("led", [](uint8_t gpio, const void*) {
        return gpio < 48; // N'importe quel GPIO valide
    });
    
    // MUX : validation complexe via MuxValidator
    registerValidator("mux", [](uint8_t gpio, const void* config) {
        if (config == nullptr) {
            // Validation basique : juste vérifier que c'est une pin ADC
            return PinMapper::hasAdc(gpio);
        }
        // Validation complète avec la config MUX
        const MuxConfig* muxCfg = static_cast<const MuxConfig*>(config);
        auto result = MuxValidator::validatePins(
            muxCfg->sig_pin, muxCfg->s0, muxCfg->s1, muxCfg->s2, muxCfg->s3, muxCfg->en_pin
        );
        return result.valid;
    });
}
