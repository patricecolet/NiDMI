#pragma once

#include "../ComponentDefinition.h"
#include "../../utils/PinMapper.h"

/**
 * @file LedDef.h
 * @brief Définition du composant LED
 * 
 * Composant de sortie PWM simple.
 * Reçoit des messages MIDI et contrôle la luminosité d'une LED via PWM.
 */

namespace Components {

/**
 * @brief Constantes pour la LED
 */
struct Led {
    // Identifiants
    static constexpr const char* ID = "led";
    static constexpr const char* DISPLAY_NAME = "LED";
    
    // Configuration
    static constexpr ComponentType TYPE = ComponentType::LED;
    static constexpr PinType PIN_TYPE = PinType::PIN_PWM;
    static constexpr bool IMPLEMENTED = true;
    static constexpr bool IS_COMPLEX = false;
    
    // Valeurs par défaut
    static constexpr uint8_t DEFAULT_CC = 1;
    static constexpr uint8_t DEFAULT_CHANNEL = 1;
    static constexpr uint16_t PWM_FREQUENCY = 5000;
    static constexpr uint8_t PWM_RESOLUTION = 8;  // 8 bits = 0-255
    
    /**
     * @brief Validation inline pour la LED
     * @param gpio GPIO à valider
     * @return true si le GPIO a une capacité PWM
     */
    static bool validate(uint8_t gpio) {
        return PinMapper::hasPwm(gpio);
    }
    
    /**
     * @brief Crée la définition pour le registre
     */
    static ComponentDefinition createDefinition() {
        return {
            ID,
            DISPLAY_NAME,
            nullptr,
            TYPE,
            PIN_TYPE,
            IMPLEMENTED,
            IS_COMPLEX
        };
    }
};

} // namespace Components
