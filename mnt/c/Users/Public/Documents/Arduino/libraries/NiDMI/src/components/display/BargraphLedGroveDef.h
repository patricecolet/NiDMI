#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageFactory.h"
#include "../../utils/PinMapper.h"

/**
 * @file BargraphLedGroveDef.h
 * @brief Bargraph LED Grove (10 segments)
 *
 * Afficheur à barres avec 10 LEDs. Reçoit des valeurs MIDI (CC ou Note velocity)
 * et allume un nombre proportionnel de segments (0-10).
 *
 * Famille: SCREEN
 * Note: Pour l'instant, utilise le processeur LED avec un seul GPIO.
 *       Une version complète pilotera plusieurs GPIOs ou un driver I2C.
 */

namespace Components {

struct BargraphLedGrove {
    // Identifiants
    static constexpr const char* ID = "bargraph_led_grove";
    static constexpr const char* DISPLAY_NAME = "Bargraph LED (Grove)";
    static constexpr const char* FAMILY_NAME = "Screen";

    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::SCREEN;
    static constexpr ComponentType TYPE = ComponentType::BARGRAPH;
    static constexpr PinType PIN_TYPE = PinType::PIN_DIGITAL;
    static constexpr bool IMPLEMENTED = true;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC = false;

    static bool validate(uint8_t gpio) {
        return gpio < 48;
    }

    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardBargraph")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeInfoField(
                "bargraphInfo",
                "Bargraph LED Grove (10 segments). Valeur MIDI 0-127 → 0-10 segments allumés. Version simplifiée : pilote un seul GPIO pour l'instant."
            ))
            .addMidiMessage(createNoteMessage())
            .addMidiMessage(createCcMessage(true, R"(["bargraph"])"))
            .build();
    }
};

} // namespace Components
