#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"

/**
 * @file InductiveProximityDef.h
 * @brief Capteur de proximité inductif (détection métal, Grove)
 *
 * Famille: DISTANCE
 * Interface typique: sortie digitale (0/1)
 * Statut: non implémenté (placeholder pour mapping pin/famille).
 */

namespace Components {

struct InductiveProximity {
    // Identifiants
    static constexpr const char* ID = "inductive_proximity";
    static constexpr const char* DISPLAY_NAME = "Proximité inductive (Grove)";
    static constexpr const char* FAMILY_NAME = "Distance";

    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::DISTANCE;
    static constexpr ComponentType  TYPE   = ComponentType::BUTTON; // binaire (détection oui/non)
    static constexpr PinType        PIN_TYPE = PinType::PIN_DIGITAL;
    static constexpr bool IMPLEMENTED    = false;
    static constexpr bool SUPPORTS_MIDI  = true;
    static constexpr bool SUPPORTS_OSC   = true;

    static bool validate(uint8_t gpio) {
        return gpio < 48;
    }

    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardInductiveProximity")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeInfoField(
                "inductiveInfo",
                "Capteur de proximité inductif (détection métal) Grove. Non implémenté pour l’instant."
            ))
            .build();
    }
};

} // namespace Components

