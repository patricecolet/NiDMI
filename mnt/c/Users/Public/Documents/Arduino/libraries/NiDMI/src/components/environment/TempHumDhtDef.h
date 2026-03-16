#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageFactory.h"
#include "../../utils/PinMapper.h"

/**
 * @file TempHumDhtDef.h
 * @brief Capteur de température & humidité type DHT (Grove)
 *
 * Famille: ENVIRONMENT
 * Interface: une pin digitale (DHT11/DHT22/AM2302...)
 * Statut: non implémenté (placeholder).
 */

namespace Components {

struct TempHumDht {
    // Identifiants
    static constexpr const char* ID = "temp_hum_dht";
    static constexpr const char* DISPLAY_NAME = "Temp/Hum (DHT, Grove)";
    static constexpr const char* FAMILY_NAME = "Environment";

    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::ENVIRONMENT;
    static constexpr ComponentType  TYPE   = ComponentType::POTENTIOMETER; // valeur continue mappée
    static constexpr PinType        PIN_TYPE = PinType::PIN_DIGITAL;
    static constexpr bool IMPLEMENTED = false;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC  = true;

    static bool validate(uint8_t gpio) {
        // TODO: éventuellement restreindre selon les broches recommandées
        return gpio < 48;
    }

    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardTempHumDht")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeInfoField(
                "tempHumDhtInfo",
                "Capteur température/humidité DHT (Grove). Non implémenté pour l’instant."
            ))
            .build();
    }
};

} // namespace Components
