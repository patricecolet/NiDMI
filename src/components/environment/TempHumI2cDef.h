#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageFactory.h"

/**
 * @file TempHumI2cDef.h
 * @brief Capteurs température/humidité I2C (AHT20, SHT3x, etc., Grove)
 *
 * Famille: ENVIRONMENT
 * Interface: I2C (SDA/SCL)
 * Statut: non implémenté (placeholder).
 */

namespace Components {

struct TempHumI2c {
    static constexpr const char* ID = "temp_hum_i2c";
    static constexpr const char* DISPLAY_NAME = "Temp/Hum (I2C, Grove)";
    static constexpr const char* FAMILY_NAME = "Environment";

    static constexpr ComponentFamily FAMILY = ComponentFamily::ENVIRONMENT;
    static constexpr ComponentType  TYPE   = ComponentType::POTENTIOMETER;
    static constexpr PinType        PIN_TYPE = PinType::PIN_DIGITAL;
    static constexpr bool IMPLEMENTED = false;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC  = true;

    static bool validate(uint8_t gpio) {
        // Placeholder: en pratique, ce sera câblé sur SDA/SCL
        return gpio < 48;
    }

    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardTempHumI2c")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeInfoField(
                "tempHumI2cInfo",
                "Capteurs température/humidité I2C (AHT20, SHT3x, Grove). Non implémenté pour l’instant."
            ))
            .build();
    }
};

} // namespace Components

