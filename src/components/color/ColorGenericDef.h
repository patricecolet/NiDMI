#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"

/**
 * @file ColorGenericDef.h
 * @brief Définition générique pour capteurs de couleur/lumière avancés (placeholder)
 *
 * Famille: COLOR
 * Modules visés: capteurs de couleur RGB, capteurs de lumière plein spectre, etc.
 * Statut: non implémenté (UI uniquement, grisé).
 */

namespace Components {

struct ColorGeneric {
    // Identifiants
    static constexpr const char* ID = "color_generic";
    static constexpr const char* DISPLAY_NAME = "Couleur / Lumière (Grove/Seeed)";
    static constexpr const char* FAMILY_NAME = "Color";

    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::COLOR;
    // Utilise POTENTIOMETER comme base (valeurs continues multi-canaux) – sera spécialisé plus tard
    static constexpr ComponentType TYPE = ComponentType::POTENTIOMETER;
    static constexpr PinType PIN_TYPE = PinType::PIN_ANALOG_OR_DIGITAL;
    static constexpr bool IMPLEMENTED = false;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC = true;

    static bool validate(uint8_t gpio) {
        return gpio < 48;
    }

    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardColor")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeInfoField(
                "colorInfo",
                "Capteurs de couleur/lumière Seeed/Grove - non implémenté pour l’instant."
            ))
            .build();
    }
};

} // namespace Components

