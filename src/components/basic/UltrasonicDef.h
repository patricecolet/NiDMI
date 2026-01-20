#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageFactory.h"

/**
 * @file UltrasonicDef.h
 * @brief Définition du composant Ultrasonique
 *
 * Capteur de distance ultrasonique sur pin digitale.
 * La distance (en mm) est mappée vers des messages MIDI (CC, Pitch Bend,
 * Aftertouch, Note Sweep).
 *
 * Famille: BASIC
 */

namespace Components {

struct Ultrasonic {
    // Identifiants
    static constexpr const char *ID = "ultrasonic";
    static constexpr const char *DISPLAY_NAME = "Ultrasonique";
    static constexpr const char *FAMILY_NAME = "Basic";

    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::BASIC;
    static constexpr ComponentType TYPE = ComponentType::ULTRASONIC;
    static constexpr PinType PIN_TYPE = PinType::PIN_DIGITAL;
    static constexpr bool IMPLEMENTED = true;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC = true;

    // Valeurs par défaut
    static constexpr uint16_t DEFAULT_MIN_DISTANCE_MM = 50;   // 5 cm
    static constexpr uint16_t DEFAULT_MAX_DISTANCE_MM = 2000; // 2 m
    static constexpr uint8_t DEFAULT_FILTER_INTENSITY = 5;
    static constexpr uint8_t DEFAULT_CHANNEL = 1;

    /**
     * @brief Validation : vérifie que le GPIO est utilisable en digital
     */
    static bool validate(uint8_t gpio) {
        // Pour l’ultrasonique: n’importe quel GPIO digital valide
        return gpio < 48;
    }

    /**
     * @brief Crée la définition complète pour le registre
     */
    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardUltrasonic")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            // Distance min/max en mm → réutilise potMin/potMax dans la config
            .addFormField(makeNumberFieldWithHint(
                "potMin",
                "Distance min (mm)",
                0, 4000,
                "50",
                "Distance minimale (mm) mappée sur 0 MIDI",
                60
            ))
            .addFormField(makeNumberFieldWithHint(
                "potMax",
                "Distance max (mm)",
                0, 4000,
                "2000",
                "Distance maximale (mm) mappée sur 127 MIDI",
                60
            ))
            .addFormField(makeNumberFieldWithHint(
                "filterIntensity",
                "Intensité filtrage (1-10)",
                1, 10,
                "5",
                "1=rapide, 10=stable",
                60
            ))
            // Messages MIDI disponibles (comme Potentiometer)
            .addMidiMessage(createCcMessage(true, "[\"ultrasonic\"]"))
            .addMidiMessage(createPitchBendMessage())
            .addMidiMessage(createAftertouchMessage())
            .addMidiMessage(createNoteSweepMessage())
            .build();
    }
};

} // namespace Components

