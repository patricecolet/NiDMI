#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageFactory.h"
#include "../../utils/PinMapper.h"

/**
 * @file LedDef.h
 * @brief Définition du composant LED
 * 
 * Composant de sortie digitale simple.
 * Reçoit des messages MIDI et contrôle une LED (on/off ou PWM si disponible).
 * 
 * Famille: BASIC
 */

namespace Components {

/**
 * @brief Définition complète de la LED
 */
struct Led {
    // Identifiants
    static constexpr const char* ID = "led";
    static constexpr const char* DISPLAY_NAME = "LED";
    static constexpr const char* FAMILY_NAME = "Basic";
    
    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::BASIC;
    static constexpr ComponentType TYPE = ComponentType::LED;
    static constexpr PinType PIN_TYPE = PinType::PIN_DIGITAL;
    static constexpr bool IMPLEMENTED = true;
    static constexpr bool SUPPORTS_MIDI = true;   // Reçoit MIDI
    static constexpr bool SUPPORTS_OSC = false;
    
    // Valeurs par défaut
    static constexpr uint8_t DEFAULT_CC = 1;
    static constexpr uint8_t DEFAULT_CHANNEL = 1;
    static constexpr uint16_t PWM_FREQUENCY = 5000;
    static constexpr uint8_t PWM_RESOLUTION = 8;
    
    /**
     * @brief Validation : vérifie que le GPIO est valide
     * Note: PWM n'est pas obligatoire, mais sera utilisé si disponible et si ledMode="pwm"
     */
    static bool validate(uint8_t gpio) {
        return gpio < 48; // N'importe quel GPIO valide
    }
    
    /**
     * @brief Crée la définition complète pour le registre
     */
    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardLed")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .setStatusValueMappings("{\"ledMode\":{\"pwm\":\"PWM\",\"onoff\":\"On/Off\"}}")
            .addFormField(makeSelectField(
                "ledMode",
                "LED",
                "[{\"value\":\"onoff\",\"label\":\"On/Off\"},{\"value\":\"pwm\",\"label\":\"PWM\"}]",
                "onoff"
            ))
            .addMidiMessage(createNoteMessage())
            .addMidiMessage(createCcMessage())
            .build();
    }
};

} // namespace Components
