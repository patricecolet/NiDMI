#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageFactory.h"
#include "../../utils/PinMapper.h"

/**
 * @file SoilMoistureCapacitiveDef.h
 * @brief Capteur d’humidité du sol capacitif (Grove)
 *
 * Famille: ENVIRONMENT
 * Interface: analogique
 * Statut: non implémenté (placeholder).
 */

namespace Components {

struct SoilMoistureCapacitive {
    static constexpr const char* ID = "soil_moisture_capacitive";
    static constexpr const char* DISPLAY_NAME = "Humidité sol (capacitif, Grove)";
    static constexpr const char* FAMILY_NAME = "Environment";

    static constexpr ComponentFamily FAMILY = ComponentFamily::ENVIRONMENT;
    static constexpr ComponentType  TYPE   = ComponentType::POTENTIOMETER;
    static constexpr PinType        PIN_TYPE = PinType::PIN_ANALOG;
    static constexpr bool IMPLEMENTED = false;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC  = true;

    static bool validate(uint8_t gpio) {
        return PinMapper::hasAdc(gpio);
    }

    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardSoilMoisture")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeInfoField(
                "soilInfo",
                "Capteur d’humidité du sol capacitif (Grove). Non implémenté pour l’instant."
            ))
            .build();
    }
};

} // namespace Components

