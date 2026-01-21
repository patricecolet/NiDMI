#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"

/**
 * @file LidarTofDef.h
 * @brief Capteur de distance LiDAR / ToF (Grove, ex: VL53L0X, TF Mini)
 *
 * Famille: DISTANCE
 * Interface typique: I2C ou UART
 * Statut: non implémenté (placeholder pour mapping pin/famille).
 */

namespace Components {

struct LidarTof {
    // Identifiants
    static constexpr const char* ID = "lidar_tof";
    static constexpr const char* DISPLAY_NAME = "LiDAR / ToF (Grove)";
    static constexpr const char* FAMILY_NAME = "Distance";

    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::DISTANCE;
    static constexpr ComponentType  TYPE   = ComponentType::ULTRASONIC; // même catégorie distance
    static constexpr PinType        PIN_TYPE = PinType::PIN_DIGITAL;
    static constexpr bool IMPLEMENTED    = false;
    static constexpr bool SUPPORTS_MIDI  = true;
    static constexpr bool SUPPORTS_OSC   = true;

    static bool validate(uint8_t gpio) {
        // Placeholder: n'importe quel GPIO digital valide (UART/I2C via pins dédiées plus tard)
        return gpio < 48;
    }

    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardLidarTof")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeInfoField(
                "lidarInfo",
                "Capteurs LiDAR / ToF Seeed/Grove (ex: VL53L0X, TF Mini). Non implémenté pour l’instant."
            ))
            .build();
    }
};

} // namespace Components

