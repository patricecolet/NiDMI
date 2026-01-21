#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageFactory.h"
#include "../../utils/PinMapper.h"

/**
 * @file UvSensorGroveDef.h
 * @brief Capteur UV Grove
 *
 * Famille: ENVIRONMENT
 * Interface: analogique ou I2C selon le module
 * Statut: non implémenté (placeholder).
 */

namespace Components {

struct UvSensorGrove {
    static constexpr const char* ID = "uv_sensor_grove";
    static constexpr const char* DISPLAY_NAME = "UV (Grove)";
    static constexpr const char* FAMILY_NAME = "Environment";

    static constexpr ComponentFamily FAMILY = ComponentFamily::ENVIRONMENT;
    static constexpr ComponentType  TYPE   = ComponentType::POTENTIOMETER;
    static constexpr PinType        PIN_TYPE = PinType::PIN_ANALOG;
    static constexpr bool IMPLEMENTED = false;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC  = true;

    static bool validate(uint8_t gpio) {
        return PinMapper::hasAdc(gpio);
    }

    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardUvSensor")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeInfoField(
                "uvInfo",
                "Capteur UV Grove. Non implémenté pour l’instant."
            ))
            .build();
    }
};

} // namespace Components

