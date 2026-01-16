#include "MuxValidator.h"
#include "ComponentManager.h"
#include "../utils/PinMapper.h"
#include "../config/ConfigCache.h"
#include "../Globals.h"

MuxValidator::ValidationResult MuxValidator::validatePins(
    uint8_t sig, uint8_t s0, uint8_t s1, uint8_t s2, uint8_t s3, uint8_t en
) {
    ValidationResult result;
    result.valid = true;
    
    // Valider les pins GPIO (0-48 pour ESP32-C3/S3)
    if (sig > 48 || s0 > 48 || s1 > 48 || s2 > 48 || s3 > 48) {
        result.valid = false;
        result.error_message = "Pin GPIO invalide (max 48)";
        return result;
    }
    if (en != 255 && en > 48) {
        result.valid = false;
        result.error_message = "Pin EN GPIO invalide (max 48)";
        return result;
    }
    
    // Vérifier que SIG a un ADC
    if (!PinMapper::hasAdc(sig)) {
        result.valid = false;
        result.error_message = "Pin SIG " + String(sig) + " n'a pas d'ADC";
        return result;
    }
    
    return result;
}

MuxValidator::ValidationResult MuxValidator::validateThresholds(
    uint16_t analog_min, uint16_t analog_max
) {
    ValidationResult result;
    result.valid = true;
    
    // Valider les seuils
    if (analog_min >= analog_max) {
        result.valid = false;
        result.error_message = "Seuils invalides: min (" + String(analog_min) + ") >= max (" + String(analog_max) + ")";
        return result;
    }
    if (analog_max > 4095) {
        result.valid = false;
        result.error_message = "Seuil max invalide: " + String(analog_max) + " (max 4095)";
        return result;
    }
    
    return result;
}

void MuxValidator::removeExistingComponents(
    ComponentManager& manager,
    uint8_t sig, uint8_t s0, uint8_t s1, uint8_t s2, uint8_t s3, uint8_t en
) {
    // Fonction helper : supprimer composant et NVS pour une pin
    auto removeComponentAndNVS = [&manager](uint8_t gpio, const char* role) {
        if (manager.removeComponent(gpio)) {
            Serial.printf("[MuxValidator] Composant existant supprime de GPIO %d (utilise pour MUX %s)\n", gpio, role);
            // Supprimer aussi la configuration NVS de cette pin
            String pinLabel = PinMapper::gpioToLabel(gpio);
            if (pinLabel.length() > 0) {
                g_configCache.removeConfig(pinLabel);
            }
        }
    };
    
    // Supprimer les composants existants sur toutes les pins du multiplexeur
    removeComponentAndNVS(sig, "SIG");
    removeComponentAndNVS(s0, "S0");
    removeComponentAndNVS(s1, "S1");
    removeComponentAndNVS(s2, "S2");
    removeComponentAndNVS(s3, "S3");
    if (en != 255) {
        removeComponentAndNVS(en, "EN");
    }
}

void MuxValidator::normalizeMidiParams(uint8_t& cc_base, uint8_t& midi_channel) {
    if (cc_base > 127) cc_base = 127;
    if (midi_channel < 1 || midi_channel > 16) midi_channel = 1;
}
