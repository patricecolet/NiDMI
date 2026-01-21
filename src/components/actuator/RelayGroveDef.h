#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageFactory.h"
#include "../../utils/PinMapper.h"

/**
 * @file RelayGroveDef.h
 * @brief Relais Grove - actuateur on/off simple
 *
 * Famille: ACTUATOR
 * Type: ACTUATOR (sortie digitale pilotée par MIDI/OSC)
 * Remarque: pas de PWM pour le relais (on/off uniquement).
 */

namespace Components {

struct RelayGrove {
    // Identifiants
    static constexpr const char* ID = "relay_grove";
    static constexpr const char* DISPLAY_NAME = "Relais (Grove)";
    static constexpr const char* FAMILY_NAME = "Actuator";

    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::ACTUATOR;
    static constexpr ComponentType  TYPE   = ComponentType::ACTUATOR;
    static constexpr PinType        PIN_TYPE = PinType::PIN_DIGITAL;
    static constexpr bool IMPLEMENTED = true;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC  = false;

    static bool validate(uint8_t gpio) {
        // Même logique que LED: toute GPIO digitale valide
        return gpio < 48;
    }

    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardRelay")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            // Mode fixe: on/off uniquement (pas de PWM)
            .setStatusValueMappings("{\"ledMode\":{\"onoff\":\"On/Off\"}}")
            .addFormField(makeSelectField(
                "ledMode",
                "Relais",
                "[{\"value\":\"onoff\",\"label\":\"On/Off\"}]",
                "onoff"
            ))
            // Messages MIDI comme pour LED (Note et CC)
            .addMidiMessage(createNoteMessage())
            .addMidiMessage(createCcMessage(true, "[\"relay\"]"))
            .build();
    }
};

} // namespace Components

