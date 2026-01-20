#pragma once

#include "../ComponentDefinition.h"
#include "../../utils/PinMapper.h"

/**
 * @file VelostatDef.h
 * @brief Définition du composant Velostat
 * 
 * Capteur de pression/flexion analogique.
 * Envoie Note On/Off selon un seuil et Key Pressure (Polyphonic Aftertouch) pendant l'activation.
 * 
 * Famille: BASIC
 */

namespace Components {

/**
 * @brief Définition complète du Velostat
 */
struct Velostat {
    // Identifiants
    static constexpr const char* ID = "velostat";
    static constexpr const char* DISPLAY_NAME = "Velostat";
    static constexpr const char* FAMILY_NAME = "Basic";
    
    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::BASIC;
    static constexpr ComponentType TYPE = ComponentType::VELOSTAT;
    static constexpr PinType PIN_TYPE = PinType::PIN_ANALOG;
    static constexpr bool IMPLEMENTED = true;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC = true;
    
    // Valeurs par défaut
    static constexpr uint8_t DEFAULT_NOTE = 60;
    static constexpr uint8_t DEFAULT_CHANNEL = 1;
    static constexpr uint8_t DEFAULT_FILTER_INTENSITY = 5;
    static constexpr uint16_t DEFAULT_VELOCITY_THRESHOLD = 50;  // 0-4095
    static constexpr uint8_t DEFAULT_AFTERTOUCH_THRESHOLD = 4;  // 1-127
    
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
        def.cardId = "cardVelostat";
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
        
        // Seuil de vélocité (velocityThreshold)
        def.formFields[0] = FormFieldDef{
            "velocityThreshold",
            "Seuil vélocité",
            FieldType::NUMBER,
            false,
            nullptr, 0, nullptr,
            0, 4095, 1,
            nullptr, nullptr,
            "50",
            HintPosition::INLINE,
            "Seuil pour Note On (0-4095)",
            "margin-left:8px;font-size:0.9em;color:#666;",
            nullptr, nullptr,
            "r", nullptr, 60,
            nullptr, nullptr
        };
        
        // Seuil d'aftertouch (aftertouchThreshold)
        def.formFields[1] = FormFieldDef{
            "aftertouchThreshold",
            "Seuil aftertouch",
            FieldType::NUMBER,
            false,
            nullptr, 0, nullptr,
            1, 127, 1,
            nullptr, nullptr,
            "4",
            HintPosition::INLINE,
            "Sensibilité aftertouch (1-127)",
            "margin-left:8px;font-size:0.9em;color:#666;",
            nullptr, nullptr,
            "r", nullptr, 60,
            nullptr, nullptr
        };
        
        // Filter intensity
        def.formFields[2] = FormFieldDef{
            "filterIntensity",
            "Intensité filtrage (1-10)",
            FieldType::NUMBER,
            false,
            nullptr, 0, nullptr,
            1, 10, 1,
            nullptr, nullptr,
            "5",
            HintPosition::INLINE,
            "1=rapide, 10=stable",
            "margin-left:8px;font-size:0.9em;color:#666;",
            nullptr, nullptr,
            "r", nullptr, 60,
            nullptr, nullptr
        };
        
        // Allouer les messages MIDI supportés
        def.midiMessageCount = 1;
        def.midiMessagesCapacity = 1;
        def.midiMessages = new MidiMessageDef[1];
        
        // Note avec Key Pressure
        def.midiMessages[0].id = "note";
        def.midiMessages[0].displayName = "Note + Key Pressure";
        def.midiMessages[0].statusTemplate = "Note {note} + Key Press";
        def.midiMessages[0].paramCount = 2;
        def.midiMessages[0].paramsCapacity = 2;
        def.midiMessages[0].params = new MidiParamDef[2];
        def.midiMessages[0].params[0] = MidiParamDef{"midiNote", "{{t.pins.note}}:", FieldType::NUMBER, 0, 127, "60", "60", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        def.midiMessages[0].params[1] = MidiParamDef{"midiChannel", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        
        return def;
    }
};

} // namespace Components
