#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"

/**
 * @file OledI2cGroveDef.h
 * @brief Écran OLED I2C Grove (placeholder)
 *
 * Famille: SCREEN
 * Statut: non implémenté (UI uniquement, grisé).
 */

namespace Components {

struct OledI2cGrove {
    static constexpr const char* ID = "oled_i2c_grove";
    static constexpr const char* DISPLAY_NAME = "OLED I2C (Grove)";
    static constexpr const char* FAMILY_NAME = "Screen";

    static constexpr ComponentFamily FAMILY = ComponentFamily::SCREEN;
    static constexpr ComponentType TYPE = ComponentType::BARGRAPH; // Réutilise temporairement
    static constexpr PinType PIN_TYPE = PinType::PIN_DIGITAL; // I2C
    static constexpr bool IMPLEMENTED = false;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC = false;

    static bool validate(uint8_t gpio) {
        return gpio < 48;
    }

    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardOled")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeInfoField(
                "oledInfo",
                "Écran OLED I2C Grove (128x64 ou 128x32) - non implémenté pour l'instant."
            ))
            .build();
    }
};

} // namespace Components
