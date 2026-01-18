#pragma once

#include "../ComponentDefinition.h"

/**
 * @file ButtonDef.h
 * @brief Définition du composant Bouton
 * 
 * Composant d'entrée digitale simple.
 * Lit un état HIGH/LOW et envoie des messages MIDI (Note, CC, Program Change).
 * 
 * Famille: BASIC
 */

namespace Components {

/**
 * @brief Définition complète du Bouton
 */
struct Button {
    // Identifiants
    static constexpr const char* ID = "button";
    static constexpr const char* DISPLAY_NAME = "Bouton";
    static constexpr const char* FAMILY_NAME = "Basic";
    
    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::BASIC;
    static constexpr ComponentType TYPE = ComponentType::BUTTON;
    static constexpr PinType PIN_TYPE = PinType::PIN_DIGITAL;
    static constexpr bool IMPLEMENTED = true;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC = true;
    
    // Valeurs par défaut
    static constexpr uint8_t DEFAULT_NOTE = 60;  // Middle C
    static constexpr uint8_t DEFAULT_CHANNEL = 1;
    static constexpr uint8_t DEFAULT_VELOCITY = 127;
    static constexpr uint32_t DEBOUNCE_TIME_MS = 50;
    
    // Modes de fonctionnement
    static constexpr const char* MODE_PULSE = "pulse";
    static constexpr const char* MODE_PRESS_RELEASE = "press_release";
    static constexpr const char* MODE_TOGGLE = "toggle";
    
    /**
     * @brief Validation : vérifie que le GPIO est valide
     */
    static bool validate(uint8_t gpio) {
        return gpio < 48;
    }
    
    /**
     * @brief Crée la définition complète pour le registre
     */
    static ComponentDefinition createDefinition() {
        ComponentDefinition def;
        def.id = ID;
        def.displayName = DISPLAY_NAME;
        def.icon = nullptr;
        def.cardId = "cardBtn";
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
        def.formFieldCount = 2;
        def.formFieldsCapacity = 2;
        def.formFields = new FormFieldDef[2];
        
        // btnMode
        def.formFields[0] = FormFieldDef{
            "btnMode",
            "Mode bouton",
            FieldType::SELECT,
            false,
            nullptr, 0, nullptr,  // placeholder, maxLength (uint16_t), pattern
            0, 0, 0,              // min, max, step
            "[{\"value\":\"pulse\",\"label\":\"Push\"},{\"value\":\"press_release\",\"label\":\"Press/Release\"},{\"value\":\"toggle\",\"label\":\"Toggle\"}]",
            nullptr,
            "pulse",
            HintPosition::NONE, nullptr, nullptr,
            nullptr, nullptr,
            "r", nullptr, 0,
            nullptr, nullptr
        };
        
        // btnPulseTiming (conditionnel sur btnMode = "pulse")
        def.formFields[1] = FormFieldDef{
            "btnPulseTiming",
            "Timing Push",
            FieldType::SELECT,
            false,
            nullptr, 0, nullptr,  // placeholder, maxLength (uint16_t), pattern
            0, 0, 0,              // min, max, step
            "[{\"value\":\"press\",\"label\":\"Au press\"},{\"value\":\"release\",\"label\":\"Au release\"}]",
            nullptr,
            "press",
            HintPosition::NONE, nullptr, nullptr,
            "btnMode",
            "[\"pulse\"]",
            "r", nullptr, 0,
            nullptr, nullptr
        };
        
        // Allouer les messages MIDI supportés
        def.midiMessageCount = 6;
        def.midiMessagesCapacity = 6;
        def.midiMessages = new MidiMessageDef[6];
        
        // Note
        def.midiMessages[0].id = "note";
        def.midiMessages[0].displayName = "Note";
        def.midiMessages[0].statusTemplate = "Note {note}";
        def.midiMessages[0].paramCount = 3;
        def.midiMessages[0].paramsCapacity = 3;
        def.midiMessages[0].params = new MidiParamDef[3];
        def.midiMessages[0].params[0] = MidiParamDef{"rtpNote", "{{t.pins.note}}:", FieldType::NUMBER, 0, 127, "60", "60", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        def.midiMessages[0].params[1] = MidiParamDef{"rtpChan", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        def.midiMessages[0].params[2] = MidiParamDef{"rtpVel", "{{t.pins.velocity}}:", FieldType::NUMBER, 1, 127, "100", "100", nullptr, nullptr, nullptr, nullptr, nullptr, 90, "[\"button\"]"};
        
        // Control Change
        def.midiMessages[1].id = "cc";
        def.midiMessages[1].displayName = "Control Change";
        def.midiMessages[1].statusTemplate = "CC#{cc}";
        def.midiMessages[1].paramCount = 3;
        def.midiMessages[1].paramsCapacity = 3;
        def.midiMessages[1].params = new MidiParamDef[3];
        def.midiMessages[1].params[0] = MidiParamDef{"rtpCc", "{{t.pins.cc}}:", FieldType::NUMBER, 0, 127, "7", "7", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        def.midiMessages[1].params[1] = MidiParamDef{"rtpChan", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        def.midiMessages[1].params[2] = MidiParamDef{"rtpCcOnOff", "{{t.pins.values}}:", FieldType::RANGE, 0, 127, nullptr, nullptr, "127", "0", "→", nullptr, nullptr, 90, "[\"button\"]"};
        
        // Program Change
        def.midiMessages[2].id = "pc";
        def.midiMessages[2].displayName = "Program Change";
        def.midiMessages[2].statusTemplate = "PC#{pc}";
        def.midiMessages[2].paramCount = 2;
        def.midiMessages[2].paramsCapacity = 2;
        def.midiMessages[2].params = new MidiParamDef[2];
        def.midiMessages[2].params[0] = MidiParamDef{"rtpPc", "{{t.pins.program}}:", FieldType::NUMBER, 0, 127, "0", "0", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        def.midiMessages[2].params[1] = MidiParamDef{"rtpChan", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        
        // Note + vélocité
        def.midiMessages[3].id = "notevel";
        def.midiMessages[3].displayName = "Note + vélocité";
        def.midiMessages[3].statusTemplate = "Note {note} +vel";
        def.midiMessages[3].paramCount = 2;
        def.midiMessages[3].paramsCapacity = 2;
        def.midiMessages[3].params = new MidiParamDef[2];
        def.midiMessages[3].params[0] = MidiParamDef{"rtpNote", "{{t.pins.note}}:", FieldType::NUMBER, 0, 127, "60", "60", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        def.midiMessages[3].params[1] = MidiParamDef{"rtpChan", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        
        // Note (balayage)
        def.midiMessages[4].id = "notesweep";
        def.midiMessages[4].displayName = "Note (balayage)";
        def.midiMessages[4].statusTemplate = "Note {note} scan";
        def.midiMessages[4].paramCount = 4;
        def.midiMessages[4].paramsCapacity = 4;
        def.midiMessages[4].params = new MidiParamDef[4];
        def.midiMessages[4].params[0] = MidiParamDef{"rtpNoteSweep", "{{t.pins.sweep}}:", FieldType::RANGE, 0, 127, nullptr, nullptr, "48", "72", "→", nullptr, nullptr, 90, nullptr};
        def.midiMessages[4].params[1] = MidiParamDef{"rtpNoteVelFix", "{{t.pins.fixedVelocity}}:", FieldType::NUMBER, 1, 127, "100", "100", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        def.midiMessages[4].params[2] = MidiParamDef{"rtpNoteSweepAutoOffDelay", "{{t.pins.autoOff}}", FieldType::NUMBER, 0, 65535, "0", "0", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        def.midiMessages[4].params[3] = MidiParamDef{"rtpChan", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        
        // Clock
        def.midiMessages[5].id = "clock";
        def.midiMessages[5].displayName = "Clock";
        def.midiMessages[5].statusTemplate = "Clock";
        def.midiMessages[5].paramCount = 1;
        def.midiMessages[5].paramsCapacity = 1;
        def.midiMessages[5].params = new MidiParamDef[1];
        def.midiMessages[5].params[0] = MidiParamDef{"rtpClockHint", nullptr, FieldType::INFO, 0, 0, nullptr, nullptr, nullptr, nullptr, nullptr, "{{t.pins.clockHint}}", "color:#6b7280;", 0, nullptr};
        
        return def;
    }
};

} // namespace Components
