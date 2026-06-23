#pragma once

#include "../ComponentDefinition.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageCatalog.h"

/**
 * @file UltrasonicDef.h
 * @brief Définition du composant Ultrasonique
 *
 * Capteur de distance ultrasonique sur pin digitale.
 * Compatible avec HC-SR04+ (3.3V, compatible ESP32).
 * Note: Le HC-SR04 standard nécessite 5V et n'est pas compatible avec l'ESP32.
 * 
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
    static constexpr const char *FAMILY_NAME = "Distance";

    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::DISTANCE;
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
        // Distance min/max en mm → réutilise potMin/potMax dans la config
        static constexpr FormFieldDef FF[] = {
            makeNumberFieldWithHint("potMin", "Distance min (mm)", 0, 4000, "50",
                "Distance minimale (mm) mappée sur 0 MIDI", 60),
            makeNumberFieldWithHint("potMax", "Distance max (mm)", 0, 4000, "2000",
                "Distance maximale (mm) mappée sur 127 MIDI", 60),
            makeNumberFieldWithHint("filterIntensity", "Intensité filtrage (1-10)", 1, 10, "5",
                "1=rapide, 10=stable", 60),
        };
        /* cc + range, dependsOnRole ultrasonic */
        static constexpr MidiParamDef CC_RANGE[] = {
            {"midiCc",      "{{t.pins.cc}}:",        FieldType::NUMBER, 0, 127, "7", "7", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr},
            {"midiChannel", "{{t.pins.channel}}:",   FieldType::NUMBER, 1, 16,  "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr},
            {"midiCcRange", "{{t.pins.midiRange}}:", FieldType::RANGE,  0, 127, nullptr, nullptr, "0", "127", "→", nullptr, nullptr, 90, "[\"ultrasonic\"]"},
        };
        static constexpr MidiMessageDef MM[] = {
            {"cc", "Control Change", "CC#{cc}", nullptr, 3, CC_RANGE, 3},
            msgPitchBend(), msgAftertouch(), msgNoteSweep(),
        };
        return makeFlashDef(ID, DISPLAY_NAME, "cardUltrasonic", FAMILY, FAMILY_NAME, TYPE, PIN_TYPE,
                            SUPPORTS_MIDI, SUPPORTS_OSC, IMPLEMENTED, FF, 3, MM, 4);
    }
};

} // namespace Components

