#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageFactory.h"

/**
 * @file BarometerDps310Def.h
 * @brief Capteur de pression/baromètre Grove (DPS310 ou équivalent)
 *
 * Famille: ENVIRONMENT
 * Interface: I2C
 * Statut: non implémenté (placeholder).
 */

namespace Components {

struct BarometerDps310 {
    static constexpr const char* ID = "barometer_dps310";
    static constexpr const char* DISPLAY_NAME = "Baromètre (DPS310, Grove)";
    static constexpr const char* FAMILY_NAME = "Environment";

    static constexpr ComponentFamily FAMILY = ComponentFamily::ENVIRONMENT;
    static constexpr ComponentType  TYPE   = ComponentType::POTENTIOMETER;
    static constexpr PinType        PIN_TYPE = PinType::PIN_DIGITAL;
    static constexpr bool IMPLEMENTED = false;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC  = true;

    static bool validate(uint8_t gpio) {
        return gpio < 48;
    }

    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardBarometerDps310")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeInfoField(
                "barometerInfo",
                "Capteur de pression/altitude DPS310 (Grove). Non implémenté pour l’instant."
            ))
            .build();
    }
};

} // namespace Components

