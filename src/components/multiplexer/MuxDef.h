#pragma once

#include "../ComponentDefinition.h"

/**
 * @file MuxDef.h
 * @brief Définitions des Multiplexeurs Analogiques
 * 
 * Composants d'entrée complexes.
 * Gère des multiplexeurs pour lire plusieurs potentiomètres
 * via une seule pin analogique.
 * 
 * Pins requises:
 * - 1 pin analogique (SIG)
 * - 4 pins digitales (S0, S1, S2, S3) pour 16 canaux
 * - 3 pins digitales (S0, S1, S2) pour 8 canaux
 * - 1 pin digitale optionnelle (EN)
 * 
 * Ce composant est "complexe" car il nécessite MuxManager pour la gestion.
 * 
 * Famille: MULTIPLEXER
 */

namespace Components {

/**
 * @brief Base commune pour tous les multiplexeurs
 */
struct MuxBase {
    // Configuration commune
    static constexpr const char* FAMILY_NAME = "Multiplexeur";
    static constexpr ComponentFamily FAMILY = ComponentFamily::MULTIPLEXER;
    static constexpr ComponentType TYPE = ComponentType::MUX;
    static constexpr PinType PIN_TYPE = PinType::PIN_ANALOG;
    static constexpr bool IS_COMPLEX = true;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC = true;
    
    // Caractéristiques communes
    static constexpr uint8_t NO_PIN = 255;
    
    // Valeurs par défaut communes
    static constexpr uint8_t DEFAULT_CC_BASE = 1;
    static constexpr uint8_t DEFAULT_CHANNEL = 1;
    static constexpr uint8_t DEFAULT_FILTER_INTENSITY = 5;
    static constexpr uint16_t DEFAULT_ANALOG_MIN = 0;
    static constexpr uint16_t DEFAULT_ANALOG_MAX = 4095;
    
    /**
     * @brief Validation basique (SIG pin seulement)
     */
    static bool validateSigPin(uint8_t gpio);
    
    /**
     * @brief Remplit les champs communs de la définition
     */
    static void fillCommonDefinition(ComponentDefinition& def) {
        def.icon = nullptr;
        def.cardId = "cardMux";
        def.family = FAMILY;
        def.familyName = FAMILY_NAME;
        def.type = TYPE;
        def.pinType = PIN_TYPE;
        def.isComplex = IS_COMPLEX;
        def.supportsMidi = SUPPORTS_MIDI;
        def.supportsOsc = SUPPORTS_OSC;
        
        // Messages MIDI supportés (même que potentiomètre)
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
        
        // Template par défaut
        def.statusTextTemplate = nullptr;
        def.statusValueMappings = nullptr;
    }
};

/**
 * @brief Multiplexeur HC4067 - 16 canaux
 */
struct HC4067 : MuxBase {
    // Identifiants
    static constexpr const char* ID = "hc4067";
    static constexpr const char* DISPLAY_NAME = "HC4067 (16 canaux)";
    static constexpr bool IMPLEMENTED = true;
    
    // Caractéristiques spécifiques
    static constexpr uint8_t NUM_CHANNELS = 16;
    static constexpr uint8_t NUM_ADDRESS_PINS = 4;  // S0, S1, S2, S3
    
    /**
     * @brief Crée la définition pour le registre
     */
    static ComponentDefinition createDefinition() {
        ComponentDefinition def = {};
        def.id = ID;
        def.displayName = DISPLAY_NAME;
        def.implemented = IMPLEMENTED;
        
        // Remplir les champs communs
        fillCommonDefinition(def);
        
        // Pins additionnelles (S0, S1, S2, S3, EN)
        def.additionalPinCount = 5;
        def.additionalPins[0] = {"s0", "S0", PinType::PIN_DIGITAL, false, 255};
        def.additionalPins[1] = {"s1", "S1", PinType::PIN_DIGITAL, false, 255};
        def.additionalPins[2] = {"s2", "S2", PinType::PIN_DIGITAL, false, 255};
        def.additionalPins[3] = {"s3", "S3", PinType::PIN_DIGITAL, false, 255};
        def.additionalPins[4] = {"en", "Enable", PinType::PIN_DIGITAL, true, 255};
        
        // Champs de formulaire pour MUX
        // Note: Les pins additionnelles (S0-S3, EN) sont gérées séparément via additionalPins
        // Ici on définit les champs de configuration analogique
        def.formFieldCount = 4;
        
        // Min threshold
        def.formFields[0] = FormFieldDef{
            "muxMin",
            "Seuil minimum",
            FieldType::NUMBER,
            false,
            nullptr, nullptr, nullptr,
            0, 4095, 1,
            nullptr, nullptr,
            "0",
            HintPosition::NONE, nullptr, nullptr,
            nullptr, nullptr,
            "f", nullptr, 100,
            nullptr, nullptr
        };
        
        // Max threshold
        def.formFields[1] = FormFieldDef{
            "muxMax",
            "Seuil maximum",
            FieldType::NUMBER,
            false,
            nullptr, nullptr, nullptr,
            0, 4095, 1,
            nullptr, nullptr,
            "4095",
            HintPosition::NONE, nullptr, nullptr,
            nullptr, nullptr,
            "f", nullptr, 100,
            nullptr, nullptr
        };
        
        // Hint pour calibrage OSC
        def.formFields[2] = FormFieldDef{
            "_muxCalibHint",
            nullptr,
            FieldType::INFO,
            false,
            nullptr, nullptr, nullptr, 0, 0, 0,
            nullptr, nullptr,
            nullptr,
            HintPosition::BELOW,
            "Adresses OSC pour le calibrage: /mux[ID]/cal/min [CH] /mux[ID]/cal/max [CH] /mux[ID]/cal/reset [CH] (CH=0-15)",
            "hint",
            nullptr, nullptr,
            nullptr, nullptr, 0,
            nullptr, nullptr
        };
        
        // Filter intensity
        def.formFields[3] = FormFieldDef{
            "muxFilterIntensity",
            "Intensité filtrage (1-10)",
            FieldType::NUMBER,
            false,
            nullptr, nullptr, nullptr,
            1, 10, 1,
            nullptr, nullptr,
            "5",
            HintPosition::INLINE,
            "1=rapide, 10=stable",
            "margin-left:8px;font-size:0.9em;color:#666;",
            nullptr, nullptr,
            "f", nullptr, 60,
            nullptr, nullptr
        };
        
        return def;
    }
};

/**
 * @brief Multiplexeur HC4051 - 8 canaux
 */
struct HC4051 : MuxBase {
    // Identifiants
    static constexpr const char* ID = "hc4051";
    static constexpr const char* DISPLAY_NAME = "HC4051 (8 canaux)";
    static constexpr bool IMPLEMENTED = false;  // Pas encore implémenté
    
    // Caractéristiques spécifiques
    static constexpr uint8_t NUM_CHANNELS = 8;
    static constexpr uint8_t NUM_ADDRESS_PINS = 3;  // S0, S1, S2
    
    /**
     * @brief Crée la définition pour le registre
     */
    static ComponentDefinition createDefinition() {
        ComponentDefinition def = {};
        def.id = ID;
        def.displayName = DISPLAY_NAME;
        def.implemented = IMPLEMENTED;
        
        // Remplir les champs communs
        fillCommonDefinition(def);
        
        // Pins additionnelles (S0, S1, S2, EN) - pas de S3 pour 8 canaux
        def.additionalPinCount = 4;
        def.additionalPins[0] = {"s0", "S0", PinType::PIN_DIGITAL, false, 255};
        def.additionalPins[1] = {"s1", "S1", PinType::PIN_DIGITAL, false, 255};
        def.additionalPins[2] = {"s2", "S2", PinType::PIN_DIGITAL, false, 255};
        def.additionalPins[3] = {"en", "Enable", PinType::PIN_DIGITAL, true, 255};
        
        // Champs de formulaire (même que HC4067)
        def.formFieldCount = 4;
        
        // Min threshold
        def.formFields[0] = FormFieldDef{
            "muxMin",
            "Seuil minimum",
            FieldType::NUMBER,
            false,
            nullptr, nullptr, nullptr,
            0, 4095, 1,
            nullptr, nullptr,
            "0",
            HintPosition::NONE, nullptr, nullptr,
            nullptr, nullptr,
            "f", nullptr, 100,
            nullptr, nullptr
        };
        
        // Max threshold
        def.formFields[1] = FormFieldDef{
            "muxMax",
            "Seuil maximum",
            FieldType::NUMBER,
            false,
            nullptr, nullptr, nullptr,
            0, 4095, 1,
            nullptr, nullptr,
            "4095",
            HintPosition::NONE, nullptr, nullptr,
            nullptr, nullptr,
            "f", nullptr, 100,
            nullptr, nullptr
        };
        
        // Hint pour calibrage OSC
        def.formFields[2] = FormFieldDef{
            "_muxCalibHint",
            nullptr,
            FieldType::INFO,
            false,
            nullptr, nullptr, nullptr, 0, 0, 0,
            nullptr, nullptr,
            nullptr,
            HintPosition::BELOW,
            "Adresses OSC pour le calibrage: /mux[ID]/cal/min [CH] /mux[ID]/cal/max [CH] /mux[ID]/cal/reset [CH] (CH=0-15)",
            "hint",
            nullptr, nullptr,
            nullptr, nullptr, 0,
            nullptr, nullptr
        };
        
        // Filter intensity
        def.formFields[3] = FormFieldDef{
            "muxFilterIntensity",
            "Intensité filtrage (1-10)",
            FieldType::NUMBER,
            false,
            nullptr, nullptr, nullptr,
            1, 10, 1,
            nullptr, nullptr,
            "5",
            HintPosition::INLINE,
            "1=rapide, 10=stable",
            "margin-left:8px;font-size:0.9em;color:#666;",
            nullptr, nullptr,
            "f", nullptr, 60,
            nullptr, nullptr
        };
        
        return def;
    }
};

} // namespace Components
