#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"

/**
 * @file IrDistanceSharpDef.h
 * @brief Capteur de distance infrarouge type Sharp (Grove)
 *
 * Famille: DISTANCE
 * Interface typique: sortie analogique (distance approximative)
 * Statut: non implémenté (placeholder pour mapping pin/famille).
 */

namespace Components {

struct IrDistanceSharp {
    // Identifiants
    static constexpr const char* ID = "ir_distance_sharp";
    static constexpr const char* DISPLAY_NAME = "Distance IR (Sharp/Grove)";
    static constexpr const char* FAMILY_NAME = "Distance";

    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::DISTANCE;
    static constexpr ComponentType  TYPE   = ComponentType::ULTRASONIC; // même catégorie logique
    static constexpr PinType        PIN_TYPE = PinType::PIN_ANALOG;
    static constexpr bool IMPLEMENTED    = false;
    static constexpr bool SUPPORTS_MIDI  = true;
    static constexpr bool SUPPORTS_OSC   = true;

    static bool validate(uint8_t gpio) {
        // Pour l'instant: accepter n'importe quel GPIO analogique valide
        return gpio < 48;
    }

    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardIrDistanceSharp")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeInfoField(
                "irSharpInfo",
                "Capteur de distance infrarouge Sharp (Grove). Non implémenté pour l’instant."
            ))
            .build();
    }
};

} // namespace Components

