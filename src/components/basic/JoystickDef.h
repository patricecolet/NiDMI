#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageFactory.h"
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
    
    JoystickConfig() 
        : filter_intensity(5)
        , joyXMin(200), joyXZeroMin(1900), joyXZeroMax(2100), joyXMax(4000)
        , joyYMin(200), joyYZeroMin(1900), joyYZeroMax(2100), joyYMax(4000)
        , invertX(false), invertY(false)
        , xMsgType(MidiMessageType::CONTROL_CHANGE), yMsgType(MidiMessageType::CONTROL_CHANGE)
        , xMidiParam(1), yMidiParam(2)
        , xMidiChannel(1), yMidiChannel(1)
        , xNoteSweepMin(48), xNoteSweepMax(72)
        , yNoteSweepMin(48), yNoteSweepMax(72) {}
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
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardJoystick")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            
            // Pin principal (axe X)
            // Note: Le pin Y sera ajouté via additionalPins
            
            // Seuils axe X
            .addFormField(makeNumberField("xMin", "X Min", 0, 4095, "200", 1, 100, "f"))
            .addFormField(makeNumberField("xZeroMin", "X Zero Min", 0, 4095, "1900", 1, 100, "f"))
            .addFormField(makeNumberField("xZeroMax", "X Zero Max", 0, 4095, "2100", 1, 100, "f"))
            .addFormField(makeNumberField("xMax", "X Max", 0, 4095, "4000", 1, 100, "f"))
            
            // Seuils axe Y
            .addFormField(makeNumberField("yMin", "Y Min", 0, 4095, "200", 1, 100, "f"))
            .addFormField(makeNumberField("yZeroMin", "Y Zero Min", 0, 4095, "1900", 1, 100, "f"))
            .addFormField(makeNumberField("yZeroMax", "Y Zero Max", 0, 4095, "2100", 1, 100, "f"))
            .addFormField(makeNumberField("yMax", "Y Max", 0, 4095, "4000", 1, 100, "f"))
            
            // Inversion d'axes
            .addFormField(makeSelectField(
                "invertX",
                "Inverser axe X",
                "[{\"value\":\"0\",\"label\":\"Normal\"},{\"value\":\"1\",\"label\":\"Inversé\"}]",
                "0",
                "r"
            ))
            .addFormField(makeSelectField(
                "invertY",
                "Inverser axe Y",
                "[{\"value\":\"0\",\"label\":\"Normal\"},{\"value\":\"1\",\"label\":\"Inversé\"}]",
                "0",
                "r"
            ))
            
            // Filtrage
            .addFormField(makeNumberFieldWithHint(
                "filterIntensity",
                "Intensité filtrage (1-10)",
                1, 10, "5", "1=rapide, 10=stable", 60, "r"
            ))
            
            // Messages MIDI pour l'axe X
            .addMidiMessage(createCcMessageForAxis("x", true))
            .addMidiMessage(createPitchBendMessageForAxis("x"))
            .addMidiMessage(createAftertouchMessageForAxis("x"))
            .addMidiMessage(createNoteSweepMessageForAxis("x"))
            
            // Messages MIDI pour l'axe Y
            .addMidiMessage(createCcMessageForAxis("y", true))
            .addMidiMessage(createPitchBendMessageForAxis("y"))
            .addMidiMessage(createAftertouchMessageForAxis("y"))
            .addMidiMessage(createNoteSweepMessageForAxis("y"))
            
            // Pin additionnelle pour axe Y
            .setAdditionalPins(
                new AdditionalPinDef[]{
                    AdditionalPinDef{"joyYPin", "Pin axe Y", PinType::PIN_ANALOG, false, 255}
                },
                1
            )
            .build();
    }
};

} // namespace Components
