#include "ValidationRegistry.h"
#include "../utils/PinMapper.h"
#include "../managers/MuxValidator.h"
#include "../managers/MuxManager.h"

// Initialisation des membres statiques
ValidatorEntry ValidationRegistry::validators_[MAX_VALIDATORS] = {};
size_t ValidationRegistry::validatorCount_ = 0;

// Fonctions de validation statiques (au lieu de lambdas)
static bool validatePotentiometer(uint8_t gpio, const void* config) {
    return PinMapper::hasAdc(gpio);
}

static bool validateButton(uint8_t gpio, const void* config) {
    // Un GPIO est valide si il est dans la plage des pins du MCU
    return gpio < 48; // ESP32 a max 48 GPIOs
}

static bool validateLed(uint8_t gpio, const void* config) {
    return gpio < 48; // N'importe quel GPIO valide
}

static bool validateMux(uint8_t gpio, const void* config) {
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
}

void ValidationRegistry::registerValidator(const char* componentId, ValidatorFunc validator) {
    if (validatorCount_ >= MAX_VALIDATORS) {
        return; // Tableau plein
    }
    // Utiliser directement les pointeurs de chaînes statiques (pas de std::string)
    validators_[validatorCount_].componentId = componentId;
    validators_[validatorCount_].validator = validator;
    validatorCount_++;
}

bool ValidationRegistry::validate(const char* componentId, uint8_t gpio, const void* config) {
    // Recherche linéaire (plus simple et sans std::string temporaire)
    for (size_t i = 0; i < validatorCount_; i++) {
        if (strcmp(validators_[i].componentId, componentId) == 0) {
            return validators_[i].validator(gpio, config);
        }
    }
    // Pas de validator enregistré = invalide par défaut
    return false;
}

bool ValidationRegistry::hasValidator(const char* componentId) {
    // Recherche linéaire (plus simple et sans std::string temporaire)
    for (size_t i = 0; i < validatorCount_; i++) {
        if (strcmp(validators_[i].componentId, componentId) == 0) {
            return true;
        }
    }
    return false;
}

void ValidationRegistry::init() {
    // Potentiomètre : nécessite une pin ADC
    registerValidator("potentiometer", validatePotentiometer);
    
    // Bouton : n'importe quelle pin GPIO valide (on vérifie juste que c'est dans la plage)
    registerValidator("button", validateButton);
    
    // LED : pin digitale (PWM optionnel, utilisé si disponible et ledMode="pwm")
    registerValidator("led", validateLed);
    
    // MUX : validation complexe via MuxValidator
    registerValidator("mux", validateMux);
}
