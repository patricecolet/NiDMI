#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"

/**
 * @file LedMatrixGroveDef.h
 * @brief Matrice LED Grove (placeholder)
 *
 * Famille: SCREEN
 * Statut: non implémenté (UI uniquement, grisé).
 */

namespace Components {

struct LedMatrixGrove {
    static constexpr const char* ID = "led_matrix_grove";
    static constexpr const char* DISPLAY_NAME = "LED Matrix (Grove)";
    static constexpr const char* FAMILY_NAME = "Screen";

    static constexpr ComponentFamily FAMILY = ComponentFamily::SCREEN;
    static constexpr ComponentType TYPE = ComponentType::BARGRAPH;
    static constexpr PinType PIN_TYPE = PinType::PIN_DIGITAL; // I2C ou SPI selon modèle
    static constexpr bool IMPLEMENTED = false;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC = false;

    static bool validate(uint8_t gpio) {
        return gpio < 48;
    }

    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardMatrix")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeInfoField(
                "matrixInfo",
                "Matrice LED Grove (8x8 ou autres formats) - non implémenté pour l'instant."
            ))
            .build();
    }
};

} // namespace Components
