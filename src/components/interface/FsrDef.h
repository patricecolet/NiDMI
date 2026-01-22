#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageFactory.h"
#include "../../utils/PinMapper.h"

/**
 * @file FsrDef.h
 * @brief Capteur de force FSR (Force Sensitive Resistor) - Grove
 *
 * Lit une valeur analogique (pression/force) et la mappe vers des messages MIDI
 * (CC, Pitch Bend, Aftertouch, Note Sweep) via le PotentiometerProcessor.
 *
 * Famille: INTERFACE
 */

namespace Components {

struct Fsr {
    // Identifiants
    static constexpr const char* ID = "fsr_grove";
    static constexpr const char* DISPLAY_NAME = "FSR (Force Sensitive Resistor)";
    static constexpr const char* FAMILY_NAME = "Interface";

    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::INTERFACE;
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
            .setBasicInfo(ID, DISPLAY_NAME, "cardForce")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            // Seuils analogiques min/max (force faible/forte) → réutilise potMin/potMax
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
            .addMidiMessage(createCcMessage(true, "[\"force\"]"))
            .addMidiMessage(createPcMessage())
            .addMidiMessage(createPitchBendMessage())
            .addMidiMessage(createAftertouchMessage())
            .addMidiMessage(createNoteSweepMessage())
            .build();
    }
};

} // namespace Components
