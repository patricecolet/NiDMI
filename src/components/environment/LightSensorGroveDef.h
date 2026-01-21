#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageFactory.h"
#include "../../utils/PinMapper.h"

/**
 * @file LightSensorGroveDef.h
 * @brief Capteur de lumière Grove (entrée analogique simple)
 *
 * Lit une valeur analogique (luminosité) et la mappe vers des messages MIDI
 * (CC, Pitch Bend, Aftertouch, Note Sweep) via le PotentiometerProcessor.
 *
 * Famille: ENVIRONMENT
 */

namespace Components {

struct LightSensorGrove {
    // Identifiants
    static constexpr const char* ID = "light_grove";
    static constexpr const char* DISPLAY_NAME = "Lumière (Grove)";
    static constexpr const char* FAMILY_NAME = "Environment";

    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::ENVIRONMENT;
    static constexpr ComponentType  TYPE   = ComponentType::POTENTIOMETER;   // réutilise le processeur potar
    static constexpr PinType        PIN_TYPE = PinType::PIN_ANALOG;
    static constexpr bool IMPLEMENTED = true;   // Composant actif
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC  = true;

    /**
     * @brief Validation : vérifie que le GPIO a une capacité ADC
     */
    static bool validate(uint8_t gpio) {
        return PinMapper::hasAdc(gpio);
    }

    /**
     * @brief Crée la définition complète pour le registre
     */
    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardLight")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            // Seuils analogiques min/max (lux faible/fort) → réutilise potMin/potMax
            .addFormField(makeNumberField(
                "potMin",
                "Seuil minimum (analogique)",
                0, 4095, "0",
                1, 100,
                "f"
            ))
            .addFormField(makeNumberField(
                "potMax",
                "Seuil maximum (analogique)",
                0, 4095, "4095",
                1, 100,
                "f"
            ))
            .addFormField(makeNumberFieldWithHint(
                "filterIntensity",
                "Intensité filtrage (1-10)",
                1, 10, "5",
                "1=rapide, 10=stable",
                60,
                "r"
            ))
            // Messages MIDI disponibles (comme Potentiometer)
            .addMidiMessage(createCcMessage(true, "[\"light\"]"))
            .addMidiMessage(createPcMessage())
            .addMidiMessage(createPitchBendMessage())
            .addMidiMessage(createAftertouchMessage())
            .addMidiMessage(createNoteSweepMessage())
            .build();
    }
};

} // namespace Components

