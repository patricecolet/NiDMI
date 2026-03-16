#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"

/**
 * @file ActuatorGenericDef.h
 * @brief Définition générique pour actionneurs (relais, moteurs, servos, buzzers) (placeholder)
 *
 * Famille: ACTUATOR
 * Statut: non implémenté (UI uniquement, grisé).
 */

namespace Components {

struct ActuatorGeneric {
    // Identifiants
    static constexpr const char* ID = "actuator_generic";
    static constexpr const char* DISPLAY_NAME = "Actionneur (Grove/Seeed)";
    static constexpr const char* FAMILY_NAME = "Actuator";

    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::ACTUATOR;
    // Utilise LED comme base (sortie) – sera spécialisé plus tard
    static constexpr ComponentType TYPE = ComponentType::LED;
    static constexpr PinType PIN_TYPE = PinType::PIN_PWM;
    static constexpr bool IMPLEMENTED = false;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC = false;

    static bool validate(uint8_t gpio) {
        return gpio < 48;
    }

    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardActuator")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeInfoField(
                "actuatorInfo",
                "Actionneurs Seeed/Grove (relais, moteurs, servos, buzzers) - non implémenté pour l’instant."
            ))
            .build();
    }
};

} // namespace Components

