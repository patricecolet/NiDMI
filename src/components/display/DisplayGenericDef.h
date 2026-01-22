#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"

/**
 * @file DisplayGenericDef.h
 * @brief Définition générique pour affichages (matrices LED, écrans) (placeholder)
 *
 * Famille: DISPLAY
 * Statut: non implémenté (UI uniquement, grisé).
 */

namespace Components {

struct DisplayGeneric {
    // Identifiants
    static constexpr const char* ID = "display_generic";
    static constexpr const char* DISPLAY_NAME = "Affichage (Grove/Seeed)";
    static constexpr const char* FAMILY_NAME = "Screen";

    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::SCREEN;
    // Utilise LED comme base (sortie) – sera spécialisé plus tard
    static constexpr ComponentType TYPE = ComponentType::LED;
    static constexpr PinType PIN_TYPE = PinType::PIN_DIGITAL;
    static constexpr bool IMPLEMENTED = false;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC = false;

    static bool validate(uint8_t gpio) {
        return gpio < 48;
    }

    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardDisplay")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeInfoField(
                "displayInfo",
                "Affichages Seeed/Grove (matrices LED, petits écrans) - non implémenté pour l’instant."
            ))
            .build();
    }
};

} // namespace Components

