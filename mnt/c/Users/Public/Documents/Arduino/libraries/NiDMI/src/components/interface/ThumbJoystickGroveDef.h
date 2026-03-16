#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageFactory.h"
#include "../../utils/PinMapper.h"

/**
 * @file ThumbJoystickGroveDef.h
 * @brief Joystick analogique (2 axes + clic) Grove
 *
 * Famille: INTERFACE
 * Statut: non implémenté (placeholder, grisé).
 */

namespace Components {

struct ThumbJoystickGrove {
    static constexpr const char* ID = "thumb_joystick_grove";
    static constexpr const char* DISPLAY_NAME = "Joystick (Grove)";
    static constexpr const char* FAMILY_NAME = "Interface";

    static constexpr ComponentFamily FAMILY = ComponentFamily::INTERFACE;
    static constexpr ComponentType  TYPE   = ComponentType::POTENTIOMETER; // axes analogiques mappés plus tard
    static constexpr PinType        PIN_TYPE = PinType::PIN_ANALOG_OR_DIGITAL;
    static constexpr bool IMPLEMENTED = false;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC  = true;

    static bool validate(uint8_t gpio) {
        // Placeholder: en pratique 2 ADC + 1 digital
        return gpio < 48;
    }

    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardJoystick")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeInfoField(
                "joystickInfo",
                "Joystick analogique 2 axes + clic (Grove). Non implémenté pour l’instant."
            ))
            .build();
    }
};

} // namespace Components

