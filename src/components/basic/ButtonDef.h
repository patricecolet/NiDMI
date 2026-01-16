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
    static constexpr bool IS_COMPLEX = false;
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
        ComponentDefinition def = {};
        def.id = ID;
        def.displayName = DISPLAY_NAME;
        def.icon = nullptr;
        def.cardId = "cardBtn";
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
        def.midiMessageCount = 6;
        def.midiMessages[0] = {"note", "Note", "Note {note}"};
        def.midiMessages[1] = {"cc", "Control Change", "CC#{cc}"};
        def.midiMessages[2] = {"pc", "Program Change", "PC#{pc}"};
        def.midiMessages[3] = {"notevel", "Note + vélocité", "Note {note} +vel"};
        def.midiMessages[4] = {"notesweep", "Note (balayage)", "Note {note} scan"};
        def.midiMessages[5] = {"clock", "Clock", "Clock"};
        
        // Template par défaut si pas de MIDI configuré
        def.statusTextTemplate = nullptr;
        def.statusValueMappings = nullptr;
        
        // Champs de formulaire
        def.formFieldCount = 2;
        
        // btnMode
        def.formFields[0] = FormFieldDef{
            "btnMode",
            "Mode bouton",
            FieldType::SELECT,
            false,
            nullptr, nullptr, nullptr, 0, 0, 0,
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
            nullptr, nullptr, nullptr, 0, 0, 0,
            "[{\"value\":\"press\",\"label\":\"Au press\"},{\"value\":\"release\",\"label\":\"Au release\"}]",
            nullptr,
            "press",
            HintPosition::NONE, nullptr, nullptr,
            "btnMode",
            "[\"pulse\"]",
            "r", nullptr, 0,
            nullptr, nullptr
        };
        
        return def;
    }
};

} // namespace Components
