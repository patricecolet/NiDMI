#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageFactory.h"
#include "../../utils/PinMapper.h"

/**
 * @file BuzzerGroveDef.h
 * @brief Buzzer Grove - actuateur sonore simple (on/off)
 *
 * Famille: ACTUATOR
 * Statut: non implémenté (placeholder, grisé).
 */

namespace Components {

struct BuzzerGrove {
    static constexpr const char* ID = "buzzer_grove";
    static constexpr const char* DISPLAY_NAME = "Buzzer (Grove)";
    static constexpr const char* FAMILY_NAME = "Actuator";

    static constexpr ComponentFamily FAMILY = ComponentFamily::ACTUATOR;
    static constexpr ComponentType  TYPE   = ComponentType::ACTUATOR;
    static constexpr PinType        PIN_TYPE = PinType::PIN_DIGITAL;
    static constexpr bool IMPLEMENTED = false;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC  = false;

    static bool validate(uint8_t gpio) {
        return gpio < 48;
    }

    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardBuzzer")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeInfoField(
                "buzzerInfo",
                "Buzzer Grove utilisé comme actuateur sonore simple (on/off). Non implémenté pour l’instant."
            ))
            .build();
    }
};

} // namespace Components

