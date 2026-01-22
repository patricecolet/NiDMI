#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"

/**
 * @file EnvironmentGenericDef.h
 * @brief Définition générique pour capteurs d’environnement Seeed/Grove (placeholder)
 *
 * Famille: ENVIRONMENT
 * Modules visés: température/humidité, pression, lumière, UV, humidité du sol, etc.
 * Statut: non implémenté (UI uniquement, grisé).
 */

namespace Components {

struct EnvironmentGeneric {
    // Identifiants
    static constexpr const char* ID = "environment_generic";
    static constexpr const char* DISPLAY_NAME = "Environnement (Grove/Seeed)";
    static constexpr const char* FAMILY_NAME = "Environment";

    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::ENVIRONMENT;
    // Utilise POTENTIOMETER comme base (valeurs continues) – sera spécialisé plus tard
    static constexpr ComponentType TYPE = ComponentType::POTENTIOMETER;
    static constexpr PinType PIN_TYPE = PinType::PIN_ANALOG_OR_DIGITAL;
    static constexpr bool IMPLEMENTED = false;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC = true;

    static bool validate(uint8_t gpio) {
        // Placeholder: n'importe quel GPIO valide pour l'instant
        return gpio < 48;
    }

    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardEnvironment")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeInfoField(
                "envInfo",
                "Capteurs environnementaux Seeed/Grove (température, humidité, pression, lumière, UV, sol) - non implémenté pour l’instant."
            ))
            .build();
    }
};

} // namespace Components

