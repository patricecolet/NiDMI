#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageFactory.h"

/**
 * @file GestureIrDef.h
 * @brief Capteur de gestes infrarouges (PAJ7620/PAJ7660, Grove)
 *
 * Famille: MOTION
 * Interface: I2C
 * Statut: non implémenté (placeholder).
 */

namespace Components {

struct GestureIr {
    static constexpr const char* ID = "gesture_ir_grove";
    static constexpr const char* DISPLAY_NAME = "Gestes IR (Grove)";
    static constexpr const char* FAMILY_NAME = "Motion";

    static constexpr ComponentFamily FAMILY = ComponentFamily::MOTION;
    static constexpr ComponentType  TYPE   = ComponentType::POTENTIOMETER; // mapping de gestes vers valeurs / notes
    static constexpr PinType        PIN_TYPE = PinType::PIN_DIGITAL;
    static constexpr bool IMPLEMENTED = false;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC  = true;

    static bool validate(uint8_t gpio) {
        return gpio < 48;
    }

    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardGestureIr")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeInfoField(
                "gestureInfo",
                "Capteur de gestes infrarouges (PAJ7620/PAJ7660, Grove). Non implémenté pour l’instant."
            ))
            .build();
    }
};

} // namespace Components

