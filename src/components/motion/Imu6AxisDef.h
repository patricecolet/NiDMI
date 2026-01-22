#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageFactory.h"

/**
 * @file Imu6AxisDef.h
 * @brief IMU 6 axes (accéléromètre + gyroscope, Grove)
 *
 * Famille: MOTION
 * Interface: I2C
 * Statut: non implémenté (placeholder).
 */

namespace Components {

struct Imu6Axis {
    static constexpr const char* ID = "imu_6axis_grove";
    static constexpr const char* DISPLAY_NAME = "IMU 6 axes (Grove)";
    static constexpr const char* FAMILY_NAME = "Motion";

    static constexpr ComponentFamily FAMILY = ComponentFamily::MOTION;
    static constexpr ComponentType  TYPE   = ComponentType::POTENTIOMETER; // valeurs continues mappées
    static constexpr PinType        PIN_TYPE = PinType::PIN_DIGITAL;
    static constexpr bool IMPLEMENTED = false;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC  = true;

    static bool validate(uint8_t gpio) {
        // En pratique, bus I2C; placeholder ici
        return gpio < 48;
    }

    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardImu6Axis")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeInfoField(
                "imuInfo",
                "Capteur IMU 6 axes (accéléromètre + gyroscope, Grove). Non implémenté pour l’instant."
            ))
            .build();
    }
};

} // namespace Components

