#pragma once

#include "../ComponentDefinition.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageCatalog.h"
#include "../../utils/PinMapper.h"
#include "../../midi/MidiMessageType.h"

/**
 * @file JoystickDef.h
 * @brief Définition du composant Joystick 2 axes
 * 
 * Joystick avec 2 axes analogiques (X et Y).
 * Chaque axe peut avoir un type MIDI différent et des seuils configurables.
 * 
 * Famille: BASIC
 */

namespace Components {

/**
 * @brief Configuration spécifique au joystick
 */
struct JoystickConfig {
    uint8_t filter_intensity;  // Intensité du filtrage (1-10)
    
    // Seuils axe X
    uint16_t joyXMin;          // Seuil minimum axe X (0-4095)
    uint16_t joyXZeroMin;      // Début zone morte axe X (0-4095)
    uint16_t joyXZeroMax;      // Fin zone morte axe X (0-4095)
    uint16_t joyXMax;          // Seuil maximum axe X (0-4095)
    
    // Seuils axe Y
    uint16_t joyYMin;          // Seuil minimum axe Y (0-4095)
    uint16_t joyYZeroMin;      // Début zone morte axe Y (0-4095)
    uint16_t joyYZeroMax;      // Fin zone morte axe Y (0-4095)
    uint16_t joyYMax;          // Seuil maximum axe Y (0-4095)
    
    // Inversion par axe
    bool invertX;              // true = inverser l'axe X
    bool invertY;              // true = inverser l'axe Y
    
    // Configuration MIDI par axe
    MidiMessageType xMsgType;  // Type de message MIDI pour l'axe X
    MidiMessageType yMsgType;  // Type de message MIDI pour l'axe Y
    uint8_t xMidiParam;        // Paramètre MIDI axe X (CC#, note#, etc.)
    uint8_t yMidiParam;        // Paramètre MIDI axe Y (CC#, note#, etc.)
    uint8_t xMidiChannel;      // Canal MIDI axe X (1-16)
    uint8_t yMidiChannel;      // Canal MIDI axe Y (1-16)

    uint8_t xNoteSweepMin, xNoteSweepMax;
    uint8_t yNoteSweepMin, yNoteSweepMax;

    /** Auto-off NOTE_SWEEP par axe en ms (NVS : X_midiNoteSweepAutoOffDelay, etc.). 0 = timer désactivé. */
    uint16_t xAutoOffDelay, yAutoOffDelay;

    /** Vélocité fixe NOTE_SWEEP par axe (NVS : X_midiNoteVelocityFix, etc.). 1..127. */
    uint8_t xNoteVelFix, yNoteVelFix;

    JoystickConfig() 
        : filter_intensity(5)
        , joyXMin(200), joyXZeroMin(1900), joyXZeroMax(2100), joyXMax(4000)
        , joyYMin(200), joyYZeroMin(1900), joyYZeroMax(2100), joyYMax(4000)
        , invertX(false), invertY(false)
        , xMsgType(MidiMessageType::CONTROL_CHANGE), yMsgType(MidiMessageType::CONTROL_CHANGE)
        , xMidiParam(1), yMidiParam(2)
        , xMidiChannel(1), yMidiChannel(1)
        , xNoteSweepMin(48), xNoteSweepMax(72)
        , yNoteSweepMin(48), yNoteSweepMax(72)
        , xAutoOffDelay(1000), yAutoOffDelay(1000)
        , xNoteVelFix(100), yNoteVelFix(100) {}
};

/**
 * @brief Définition complète du Joystick
 */
struct Joystick {
    // Identifiants
    static constexpr const char* ID = "joystick";
    static constexpr const char* DISPLAY_NAME = "Joystick 2 axes";
    static constexpr const char* FAMILY_NAME = "Basic";
    
    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::BASIC;
    static constexpr ComponentType TYPE = ComponentType::JOYSTICK;
    static constexpr PinType PIN_TYPE = PinType::PIN_ANALOG;
    static constexpr bool IMPLEMENTED = true;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC = true;
    
    // Valeurs par défaut
    static constexpr uint8_t DEFAULT_CHANNEL = 1;
    static constexpr uint8_t DEFAULT_FILTER_INTENSITY = 5;
    static constexpr uint16_t DEFAULT_MIN = 200;
    static constexpr uint16_t DEFAULT_ZERO_MIN = 1900;
    static constexpr uint16_t DEFAULT_ZERO_MAX = 2100;
    static constexpr uint16_t DEFAULT_MAX = 4000;
    
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
        static constexpr FormFieldDef FF[] = {
            // Seuils axe X
            makeNumberField("xMin", "X Min", 0, 4095, "200", 1, 100, "f"),
            makeNumberField("xZeroMin", "X Zero Min", 0, 4095, "1900", 1, 100, "f"),
            makeNumberField("xZeroMax", "X Zero Max", 0, 4095, "2100", 1, 100, "f"),
            makeNumberField("xMax", "X Max", 0, 4095, "4000", 1, 100, "f"),
            // Seuils axe Y
            makeNumberField("yMin", "Y Min", 0, 4095, "200", 1, 100, "f"),
            makeNumberField("yZeroMin", "Y Zero Min", 0, 4095, "1900", 1, 100, "f"),
            makeNumberField("yZeroMax", "Y Zero Max", 0, 4095, "2100", 1, 100, "f"),
            makeNumberField("yMax", "Y Max", 0, 4095, "4000", 1, 100, "f"),
            // Inversion d'axes
            makeSelectField("invertX", "Inverser axe X",
                "[{\"value\":\"0\",\"label\":\"Normal\"},{\"value\":\"1\",\"label\":\"Inversé\"}]", "0", "r"),
            makeSelectField("invertY", "Inverser axe Y",
                "[{\"value\":\"0\",\"label\":\"Normal\"},{\"value\":\"1\",\"label\":\"Inversé\"}]", "0", "r"),
            // Filtrage
            makeNumberFieldWithHint("filterIntensity", "Intensité filtrage (1-10)", 1, 10, "5", "1=rapide, 10=stable", 60, "r"),
        };
        /* cc + range pour axe (dependsOnRole null) : params partagés X/Y */
        static constexpr MidiParamDef CC_RANGE[] = {
            {"midiCc",      "{{t.pins.cc}}:",        FieldType::NUMBER, 0, 127, "7", "7", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr},
            {"midiChannel", "{{t.pins.channel}}:",   FieldType::NUMBER, 1, 16,  "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr},
            {"midiCcRange", "{{t.pins.midiRange}}:", FieldType::RANGE,  0, 127, nullptr, nullptr, "0", "127", "→", nullptr, nullptr, 90, nullptr},
        };
        static constexpr MidiMessageDef MM[] = {
            // Axe X
            {"cc",         "Control Change (Axe X)", "CC#{cc}",          "x", 3, CC_RANGE,           3},
            {"pitchbend",  "Pitch Bend (Axe X)",     "Pitch Bend",       "x", 1, PARAM_CHANNEL_ONLY, 1},
            {"aftertouch", "Aftertouch (Axe X)",     "Aftertouch",       "x", 1, PARAM_CHANNEL_ONLY, 1},
            {"notesweep",  "Note (balayage) (Axe X)","Note {note} scan", "x", 4, PARAM_NOTESWEEP,    4},
            // Axe Y
            {"cc",         "Control Change (Axe Y)", "CC#{cc}",          "y", 3, CC_RANGE,           3},
            {"pitchbend",  "Pitch Bend (Axe Y)",     "Pitch Bend",       "y", 1, PARAM_CHANNEL_ONLY, 1},
            {"aftertouch", "Aftertouch (Axe Y)",     "Aftertouch",       "y", 1, PARAM_CHANNEL_ONLY, 1},
            {"notesweep",  "Note (balayage) (Axe Y)","Note {note} scan", "y", 4, PARAM_NOTESWEEP,    4},
        };
        /* Pin additionnelle pour axe Y */
        static constexpr AdditionalPinDef AP[] = {
            {"joyYPin", "Pin axe Y", PinType::PIN_ANALOG, false, 255},
        };
        return makeFlashDef(ID, DISPLAY_NAME, "cardJoystick", FAMILY, FAMILY_NAME, TYPE, PIN_TYPE,
                            SUPPORTS_MIDI, SUPPORTS_OSC, IMPLEMENTED, FF, 11, MM, 8,
                            nullptr, AP, 1);
    }
};

} // namespace Components
