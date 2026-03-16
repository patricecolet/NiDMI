#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"

/**
 * @file Lcd16x2I2cGroveDef.h
 * @brief LCD 16x2 I2C Grove (placeholder)
 *
 * Famille: SCREEN
 * Statut: non implémenté (UI uniquement, grisé).
 */

namespace Components {

struct Lcd16x2I2cGrove {
    static constexpr const char* ID = "lcd_16x2_i2c_grove";
    static constexpr const char* DISPLAY_NAME = "LCD 16x2 I2C (Grove)";
    static constexpr const char* FAMILY_NAME = "Screen";

    static constexpr ComponentFamily FAMILY = ComponentFamily::SCREEN;
    static constexpr ComponentType TYPE = ComponentType::BARGRAPH;
    static constexpr PinType PIN_TYPE = PinType::PIN_DIGITAL; // I2C
    static constexpr bool IMPLEMENTED = false;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC = false;

    static bool validate(uint8_t gpio) {
        return gpio < 48;
    }

    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardLcd")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeInfoField(
                "lcdInfo",
                "LCD 16x2 caractères I2C Grove - non implémenté pour l'instant."
            ))
            .build();
    }
};

} // namespace Components
