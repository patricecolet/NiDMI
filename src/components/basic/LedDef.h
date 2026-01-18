#pragma once

#include "../ComponentDefinition.h"
#include "../../utils/PinMapper.h"

/**
 * @file LedDef.h
 * @brief Définition du composant LED
 * 
 * Composant de sortie digitale simple.
 * Reçoit des messages MIDI et contrôle une LED (on/off ou PWM si disponible).
 * 
 * Famille: BASIC
 */

namespace Components {

/**
 * @brief Définition complète de la LED
 */
struct Led {
    // Identifiants
    static constexpr const char* ID = "led";
    static constexpr const char* DISPLAY_NAME = "LED";
    static constexpr const char* FAMILY_NAME = "Basic";
    
    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::BASIC;
    static constexpr ComponentType TYPE = ComponentType::LED;
    static constexpr PinType PIN_TYPE = PinType::PIN_DIGITAL;
    static constexpr bool IMPLEMENTED = true;
    static constexpr bool SUPPORTS_MIDI = true;   // Reçoit MIDI
    static constexpr bool SUPPORTS_OSC = false;
    
    // Valeurs par défaut
    static constexpr uint8_t DEFAULT_CC = 1;
    static constexpr uint8_t DEFAULT_CHANNEL = 1;
    static constexpr uint16_t PWM_FREQUENCY = 5000;
    static constexpr uint8_t PWM_RESOLUTION = 8;
    
    /**
     * @brief Validation : vérifie que le GPIO est valide
     * Note: PWM n'est pas obligatoire, mais sera utilisé si disponible et si ledMode="pwm"
     */
    static bool validate(uint8_t gpio) {
        return gpio < 48; // N'importe quel GPIO valide
    }
    
    /**
     * @brief Crée la définition complète pour le registre
     */
    static ComponentDefinition createDefinition() {
        ComponentDefinition def;
        def.id = ID;
        def.displayName = DISPLAY_NAME;
        def.icon = nullptr;
        def.cardId = "cardLed";
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
        // Mapping pour ledMode dans le texte de statut
        def.statusValueMappings = "{\"ledMode\":{\"pwm\":\"PWM\",\"onoff\":\"On/Off\"}}";
        
        // Allouer les champs de formulaire
        def.formFieldCount = 1;
        def.formFieldsCapacity = 1;
        def.formFields = new FormFieldDef[1];
        def.formFields[0] = FormFieldDef{
            "ledMode",
            "LED",
            FieldType::SELECT,
            false,
            nullptr, 0, nullptr,  // placeholder, maxLength (uint16_t), pattern
            0, 0, 0,              // min, max, step
            "[{\"value\":\"onoff\",\"label\":\"On/Off\"},{\"value\":\"pwm\",\"label\":\"PWM\"}]",
            nullptr,
            "onoff",
            HintPosition::NONE, nullptr, nullptr,
            nullptr, nullptr,
            "r", nullptr, 0,
            nullptr, nullptr
        };
        
        // Allouer les messages MIDI que la LED peut recevoir
        def.midiMessageCount = 2;
        def.midiMessagesCapacity = 2;
        def.midiMessages = new MidiMessageDef[2];
        
        // Note
        def.midiMessages[0].id = "note";
        def.midiMessages[0].displayName = "Note";
        def.midiMessages[0].statusTemplate = "Note {note}";
        def.midiMessages[0].paramCount = 2;
        def.midiMessages[0].paramsCapacity = 2;
        def.midiMessages[0].params = new MidiParamDef[2];
        def.midiMessages[0].params[0] = MidiParamDef{"rtpNote", "{{t.pins.note}}:", FieldType::NUMBER, 0, 127, "60", "60", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        def.midiMessages[0].params[1] = MidiParamDef{"rtpChan", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        
        // Control Change
        def.midiMessages[1].id = "cc";
        def.midiMessages[1].displayName = "Control Change";
        def.midiMessages[1].statusTemplate = "CC#{cc}";
        def.midiMessages[1].paramCount = 2;
        def.midiMessages[1].paramsCapacity = 2;
        def.midiMessages[1].params = new MidiParamDef[2];
        def.midiMessages[1].params[0] = MidiParamDef{"rtpCc", "{{t.pins.cc}}:", FieldType::NUMBER, 0, 127, "7", "7", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        def.midiMessages[1].params[1] = MidiParamDef{"rtpChan", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        
        return def;
    }
};

} // namespace Components
