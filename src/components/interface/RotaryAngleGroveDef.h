#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageFactory.h"
#include "../../utils/PinMapper.h"

/**
 * @file RotaryAngleGroveDef.h
 * @brief Potentiomètre rotatif Grove
 *
 * Famille: INTERFACE
 * Statut: non implémenté (placeholder, grisé).
 */

namespace Components {

struct RotaryAngleGrove {
    static constexpr const char* ID = "rotary_angle_grove";
    static constexpr const char* DISPLAY_NAME = "Potentiomètre rotatif (Grove)";
    static constexpr const char* FAMILY_NAME = "Interface";

    static constexpr ComponentFamily FAMILY = ComponentFamily::INTERFACE;
    static constexpr ComponentType  TYPE   = ComponentType::POTENTIOMETER;
    static constexpr PinType        PIN_TYPE = PinType::PIN_ANALOG;
    static constexpr bool IMPLEMENTED = false;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC  = true;

    static bool validate(uint8_t gpio) {
        return PinMapper::hasAdc(gpio);
    }

    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardRotary")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeInfoField(
                "rotaryInfo",
                "Potentiomètre rotatif Grove utilisé comme contrôle continu. Non implémenté pour l’instant."
            ))
            .build();
    }
};

} // namespace Components

