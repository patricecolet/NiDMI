#pragma once

#include "../ComponentDefinition.h"
#include "../../utils/PinMapper.h"

/**
 * @file PotentiometerDef.h
 * @brief Définition du composant Potentiomètre
 * 
 * Composant d'entrée analogique simple.
 * Lit une valeur 0-4095 sur un ADC et la convertit en message MIDI.
 * 
 * Famille: BASIC
 */

namespace Components {

/**
 * @brief Définition complète du Potentiomètre
 */
struct Potentiometer {
    // Identifiants
    static constexpr const char* ID = "potentiometer";
    static constexpr const char* DISPLAY_NAME = "Potentiomètre";
    static constexpr const char* FAMILY_NAME = "Basic";
    
    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::BASIC;
    static constexpr ComponentType TYPE = ComponentType::POTENTIOMETER;
    static constexpr PinType PIN_TYPE = PinType::PIN_ANALOG;
    static constexpr bool IMPLEMENTED = true;
    static constexpr bool IS_COMPLEX = false;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC = true;
    
    // Valeurs par défaut
    static constexpr uint8_t DEFAULT_CC = 1;
    static constexpr uint8_t DEFAULT_CHANNEL = 1;
    static constexpr uint8_t DEFAULT_FILTER_INTENSITY = 5;
    
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
        ComponentDefinition def = {};
        def.id = ID;
        def.displayName = DISPLAY_NAME;
        def.icon = nullptr;
        def.cardId = "cardPot";
        def.family = FAMILY;
        def.familyName = FAMILY_NAME;
        def.type = TYPE;
        def.pinType = PIN_TYPE;
        def.implemented = IMPLEMENTED;
        def.isComplex = IS_COMPLEX;
        def.supportsMidi = SUPPORTS_MIDI;
        def.supportsOsc = SUPPORTS_OSC;
        def.additionalPinCount = 0;
        
        // Messages MIDI supportés
        def.midiMessageCount = 4;
        
        // Control Change
        def.midiMessages[0] = MidiMessageDef{
            "cc", "Control Change", "CC#{cc}", 3,
            {
                MidiParamDef{"rtpCc", "{{t.pins.cc}}:", FieldType::NUMBER, 0, 127, "7", "7", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr},
                MidiParamDef{"rtpChan", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr},
                MidiParamDef{"rtpCcRange", "{{t.pins.midiRange}}:", FieldType::RANGE, 0, 127, nullptr, nullptr, "0", "127", "→", nullptr, nullptr, 90, "[\"potentiometer\"]"}
            }
        };
        
        // Program Change
        def.midiMessages[1] = MidiMessageDef{
            "pc", "Program Change", "PC#{pc}", 2,
            {
                MidiParamDef{"rtpPc", "{{t.pins.program}}:", FieldType::NUMBER, 0, 127, "0", "0", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr},
                MidiParamDef{"rtpChan", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr}
            }
        };
        
        // Pitch Bend
        def.midiMessages[2] = MidiMessageDef{
            "pitchbend", "Pitch Bend", "Pitch Bend", 1,
            {
                MidiParamDef{"rtpChan", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr}
            }
        };
        
        // Aftertouch
        def.midiMessages[3] = MidiMessageDef{
            "aftertouch", "Aftertouch (Channel)", "Aftertouch", 1,
            {
                MidiParamDef{"rtpChan", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr}
            }
        };
        
        // Template par défaut si pas de MIDI configuré
        def.statusTextTemplate = nullptr;
        def.statusValueMappings = nullptr;
        
        // Champs de formulaire
        def.formFieldCount = 1;
        def.formFields[0] = FormFieldDef{
            "filterIntensity",
            "Intensité filtrage (1-10)",
            FieldType::NUMBER,
            false,  // required
            nullptr,  // placeholder
            0,  // maxLength
            nullptr,  // pattern
            1,  // min
            10,  // max
            1,  // step
            nullptr,  // options
            nullptr,  // separator
            "5",  // defaultValue
            HintPosition::INLINE,  // hintPosition
            "1=rapide, 10=stable",  // hint
            "margin-left:8px;font-size:0.9em;color:#666;",  // hintClass
            nullptr,  // dependsOn
            nullptr,  // showWhen
            "r",  // wrapperClass
            nullptr,  // inputClass
            60,  // width
            nullptr,  // labelBefore
            nullptr   // labelAfter
        };
        
        return def;
    }
};

} // namespace Components
