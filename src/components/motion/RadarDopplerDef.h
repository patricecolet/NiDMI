#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageFactory.h"

/**
 * @file RadarDopplerDef.h
 * @brief Capteur de mouvement radar Doppler (Grove)
 *
 * Famille: MOTION
 * Interface: sortie digitale (présence) ou analogique (vitesse)
 * Statut: non implémenté (placeholder).
 */

namespace Components {

struct RadarDoppler {
    static constexpr const char* ID = "radar_doppler_grove";
    static constexpr const char* DISPLAY_NAME = "Radar Doppler (Grove)";
    static constexpr const char* FAMILY_NAME = "Motion";

    static constexpr ComponentFamily FAMILY = ComponentFamily::MOTION;
    static constexpr ComponentType  TYPE   = ComponentType::BUTTON; // présence binaire pour commencer
    static constexpr PinType        PIN_TYPE = PinType::PIN_ANALOG_OR_DIGITAL;
    static constexpr bool IMPLEMENTED = false;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC  = true;

    static bool validate(uint8_t gpio) {
        return gpio < 48;
    }

    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardRadarDoppler")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeInfoField(
                "radarInfo",
                "Capteur radar Doppler (mouvement) Grove. Non implémenté pour l’instant."
            ))
            .build();
    }
};

} // namespace Components

