#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"

/**
 * @file MotionGenericDef.h
 * @brief Définition générique pour capteurs de mouvement Seeed/Grove (placeholder)
 *
 * Famille: MOTION
 * Modules visés: PIR, IMU, gestes IR, etc.
 * Statut: non implémenté (UI uniquement, grisé).
 */

namespace Components {

struct MotionGeneric {
    // Identifiants
    static constexpr const char* ID = "motion_generic";
    static constexpr const char* DISPLAY_NAME = "Mouvement (Grove/Seeed)";
    static constexpr const char* FAMILY_NAME = "Motion";

    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::MOTION;
    // Utilise BUTTON comme base pour les capteurs binaires (PIR) – sera spécialisé plus tard
    static constexpr ComponentType TYPE = ComponentType::BUTTON;
    static constexpr PinType PIN_TYPE = PinType::PIN_ANALOG_OR_DIGITAL;
    static constexpr bool IMPLEMENTED = false;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC = true;

    static bool validate(uint8_t gpio) {
        return gpio < 48;
    }

    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardMotion")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeInfoField(
                "motionInfo",
                "Capteurs de mouvement Seeed/Grove (PIR, IMU, gestes) - non implémenté pour l’instant."
            ))
            .build();
    }
};

} // namespace Components

