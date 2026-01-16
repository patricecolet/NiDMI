#pragma once

#include "../ComponentDefinition.h"
#include "../../utils/PinMapper.h"

/**
 * @file PotentiometerDef.h
 * @brief Définition du composant Potentiomètre
 * 
 * Composant d'entrée analogique simple.
 * Lit une valeur 0-4095 sur un ADC et la convertit en MIDI CC (0-127).
 */

namespace Components {

/**
 * @brief Constantes pour le Potentiomètre
 */
struct Potentiometer {
    // Identifiants
    static constexpr const char* ID = "potentiometer";
    static constexpr const char* DISPLAY_NAME = "Potentiomètre";
    
    // Configuration
    static constexpr ComponentType TYPE = ComponentType::POTENTIOMETER;
    static constexpr PinType PIN_TYPE = PinType::PIN_ANALOG;
    static constexpr bool IMPLEMENTED = true;
    static constexpr bool IS_COMPLEX = false;
    
    // Valeurs par défaut
    static constexpr uint8_t DEFAULT_CC = 1;
    static constexpr uint8_t DEFAULT_CHANNEL = 1;
    static constexpr uint8_t DEFAULT_FILTER_INTENSITY = 5;  // 1-10
    
    /**
     * @brief Validation inline pour le potentiomètre
     * @param gpio GPIO à valider
     * @return true si le GPIO a une capacité ADC
     */
    static bool validate(uint8_t gpio) {
        return PinMapper::hasAdc(gpio);
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
