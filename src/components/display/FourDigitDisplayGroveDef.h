#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"

/**
 * @file FourDigitDisplayGroveDef.h
 * @brief Afficheur 4 digits Grove (placeholder)
 *
 * Famille: SCREEN
 * Statut: non implémenté (UI uniquement, grisé).
 */

namespace Components {

struct FourDigitDisplayGrove {
    static constexpr const char* ID = "four_digit_display_grove";
    static constexpr const char* DISPLAY_NAME = "4-Digit Display (Grove)";
    static constexpr const char* FAMILY_NAME = "Screen";

    static constexpr ComponentFamily FAMILY = ComponentFamily::SCREEN;
    static constexpr ComponentType TYPE = ComponentType::BARGRAPH;
    static constexpr PinType PIN_TYPE = PinType::PIN_DIGITAL;
    static constexpr bool IMPLEMENTED = false;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC = false;

    static bool validate(uint8_t gpio) {
        return gpio < 48;
    }

    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "card4Digit")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeInfoField(
                "fourDigitInfo",
                "Afficheur 4 digits Grove (TM1637) - non implémenté pour l'instant."
            ))
            .build();
    }
};

} // namespace Components
