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
        ComponentDefinition def;
        def.id = ID;
        def.displayName = DISPLAY_NAME;
        def.icon = nullptr;
        def.cardId = "cardPot";
        def.family = FAMILY;
        def.familyName = FAMILY_NAME;
        def.type = TYPE;
        def.pinType = PIN_TYPE;
        def.implemented = IMPLEMENTED;
        def.supportsMidi = SUPPORTS_MIDI;
        def.supportsOsc = SUPPORTS_OSC;
        def.additionalPinCount = 0;
        def.additionalPins = nullptr;
        def.additionalPinsCapacity = 0;
        
        // Template par défaut si pas de MIDI configuré
        def.statusTextTemplate = nullptr;
        def.statusValueMappings = nullptr;
        
        // Allouer les champs de formulaire
        def.formFieldCount = 3;
        def.formFieldsCapacity = 3;
        def.formFields = new FormFieldDef[3];
        
        // Min threshold
        def.formFields[0] = FormFieldDef{
            "potMin",
            "Seuil minimum",
            FieldType::NUMBER,
            false,
            nullptr, 0, nullptr,  // placeholder, maxLength (uint16_t), pattern
            0, 4095, 1,            // min, max, step
            nullptr, nullptr,
            "0",
            HintPosition::NONE, nullptr, nullptr,
            nullptr, nullptr,
            "f", nullptr, 100,
            nullptr, nullptr
        };
        
        // Max threshold
        def.formFields[1] = FormFieldDef{
            "potMax",
            "Seuil maximum",
            FieldType::NUMBER,
            false,
            nullptr, 0, nullptr,  // placeholder, maxLength (uint16_t), pattern
            0, 4095, 1,            // min, max, step
            nullptr, nullptr,
            "4095",
            HintPosition::NONE, nullptr, nullptr,
            nullptr, nullptr,
            "f", nullptr, 100,
            nullptr, nullptr
        };
        
        // Filter intensity
        def.formFields[2] = FormFieldDef{
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
        
        // Allouer les messages MIDI supportés
        def.midiMessageCount = 4;
        def.midiMessagesCapacity = 4;
        def.midiMessages = new MidiMessageDef[4];
        
        // Control Change
        def.midiMessages[0].id = "cc";
        def.midiMessages[0].displayName = "Control Change";
        def.midiMessages[0].statusTemplate = "CC#{cc}";
        def.midiMessages[0].paramCount = 3;
        def.midiMessages[0].paramsCapacity = 3;
        def.midiMessages[0].params = new MidiParamDef[3];
        def.midiMessages[0].params[0] = MidiParamDef{"midiCc", "{{t.pins.cc}}:", FieldType::NUMBER, 0, 127, "7", "7", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        def.midiMessages[0].params[1] = MidiParamDef{"midiChannel", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        def.midiMessages[0].params[2] = MidiParamDef{"midiCcRange", "{{t.pins.midiRange}}:", FieldType::RANGE, 0, 127, nullptr, nullptr, "0", "127", "→", nullptr, nullptr, 90, "[\"potentiometer\"]"};
        
        // Program Change
        def.midiMessages[1].id = "pc";
        def.midiMessages[1].displayName = "Program Change";
        def.midiMessages[1].statusTemplate = "PC#{pc}";
        def.midiMessages[1].paramCount = 2;
        def.midiMessages[1].paramsCapacity = 2;
        def.midiMessages[1].params = new MidiParamDef[2];
        def.midiMessages[1].params[0] = MidiParamDef{"midiPc", "{{t.pins.program}}:", FieldType::NUMBER, 0, 127, "0", "0", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        def.midiMessages[1].params[1] = MidiParamDef{"midiChannel", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        
        // Pitch Bend
        def.midiMessages[2].id = "pitchbend";
        def.midiMessages[2].displayName = "Pitch Bend";
        def.midiMessages[2].statusTemplate = "Pitch Bend";
        def.midiMessages[2].paramCount = 1;
        def.midiMessages[2].paramsCapacity = 1;
        def.midiMessages[2].params = new MidiParamDef[1];
        def.midiMessages[2].params[0] = MidiParamDef{"midiChannel", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        
        // Aftertouch
        def.midiMessages[3].id = "aftertouch";
        def.midiMessages[3].displayName = "Aftertouch (Channel)";
        def.midiMessages[3].statusTemplate = "Aftertouch";
        def.midiMessages[3].paramCount = 1;
        def.midiMessages[3].paramsCapacity = 1;
        def.midiMessages[3].params = new MidiParamDef[1];
        def.midiMessages[3].params[0] = MidiParamDef{"midiChannel", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        
        return def;
    }
};

} // namespace Components
