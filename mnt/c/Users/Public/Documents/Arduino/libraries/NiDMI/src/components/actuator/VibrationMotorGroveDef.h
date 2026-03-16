#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageFactory.h"
#include "../../utils/PinMapper.h"

/**
 * @file VibrationMotorGroveDef.h
 * @brief Moteur de vibration Grove
 *
 * Famille: ACTUATOR
 * Statut: non implémenté (placeholder, grisé).
 */

namespace Components {

struct VibrationMotorGrove {
    static constexpr const char* ID = "vibration_motor_grove";
    static constexpr const char* DISPLAY_NAME = "Moteur vibration (Grove)";
    static constexpr const char* FAMILY_NAME = "Actuator";

    static constexpr ComponentFamily FAMILY = ComponentFamily::ACTUATOR;
    static constexpr ComponentType  TYPE   = ComponentType::ACTUATOR;
    static constexpr PinType        PIN_TYPE = PinType::PIN_PWM;
    static constexpr bool IMPLEMENTED = false;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC  = false;

    static bool validate(uint8_t gpio) {
        // Idéalement, vérifier capacité PWM plus tard
        return gpio < 48;
    }

    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardVibration")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeInfoField(
                "vibrationInfo",
                "Moteur de vibration Grove (intensité potentiellement pilotée en PWM). Non implémenté pour l’instant."
            ))
            .build();
    }
};

} // namespace Components

