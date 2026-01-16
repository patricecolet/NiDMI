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
 * @brief Définition complète de la LED
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
    static constexpr bool SUPPORTS_MIDI = true;   // Reçoit MIDI
    static constexpr bool SUPPORTS_OSC = false;
    
    // Valeurs par défaut
    static constexpr uint8_t DEFAULT_CC = 1;
    static constexpr uint8_t DEFAULT_CHANNEL = 1;
    static constexpr uint16_t PWM_FREQUENCY = 5000;
    static constexpr uint8_t PWM_RESOLUTION = 8;
    
    /**
     * @brief Validation : vérifie que le GPIO supporte PWM
     */
    static bool validate(uint8_t gpio) {
        return PinMapper::hasPwm(gpio);
    }
    
    /**
     * @brief Crée la définition complète pour le registre
     */
    static ComponentDefinition createDefinition() {
        ComponentDefinition def = {};
        def.id = ID;
        def.displayName = DISPLAY_NAME;
        def.icon = nullptr;
        def.cardId = "cardLed";
        def.type = TYPE;
        def.pinType = PIN_TYPE;
        def.implemented = IMPLEMENTED;
        def.isComplex = IS_COMPLEX;
        def.supportsMidi = SUPPORTS_MIDI;
        def.supportsOsc = SUPPORTS_OSC;
        def.additionalPinCount = 0;
        def.variantCount = 0;
        
        // Messages MIDI que la LED peut recevoir
        def.midiMessageCount = 2;
        def.midiMessages[0] = {"note", "Note"};
        def.midiMessages[1] = {"cc", "Control Change"};
        
        return def;
    }
};

} // namespace Components
