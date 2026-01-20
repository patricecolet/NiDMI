#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageFactory.h"

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
     * @brief Crée un builder avec les champs communs remplis
     */
    static ComponentBuilder createCommonBuilder() {
        return ComponentBuilder()
            .setBasicInfo("", "", "cardMux")  // id et displayName seront définis par les sous-classes
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .addMidiMessage(createCcMessage(true, "[\"potentiometer\"]"))
            .addMidiMessage(createPcMessage())
            .addMidiMessage(createPitchBendMessage())
            .addMidiMessage(createAftertouchMessage());
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
        AdditionalPinDef pins[5] = {
            {"s0", "S0", PinType::PIN_DIGITAL, false, 255},
            {"s1", "S1", PinType::PIN_DIGITAL, false, 255},
            {"s2", "S2", PinType::PIN_DIGITAL, false, 255},
            {"s3", "S3", PinType::PIN_DIGITAL, false, 255},
            {"en", "Enable", PinType::PIN_DIGITAL, true, 255}
        };
        
        return createCommonBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardMux")
            .setImplemented(IMPLEMENTED)
            .setAdditionalPins(pins, 5)
            .addFormField(makeNumberField("muxMin", "Seuil minimum", 0, 4095, "0", 1, 100, "f"))
            .addFormField(makeNumberField("muxMax", "Seuil maximum", 0, 4095, "4095", 1, 100, "f"))
            .addFormField(makeInfoField(
                "_muxCalibHint",
                "Adresses OSC pour le calibrage: /mux[ID]/cal/min [CH] /mux[ID]/cal/max [CH] /mux[ID]/cal/reset [CH] (CH=0-15)"
            ))
            .addFormField(makeNumberFieldWithHint(
                "muxFilterIntensity",
                "Intensité filtrage (1-10)",
                1, 10, "5", "1=rapide, 10=stable", 60, "f"
            ))
            .build();
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
        AdditionalPinDef pins[4] = {
            {"s0", "S0", PinType::PIN_DIGITAL, false, 255},
            {"s1", "S1", PinType::PIN_DIGITAL, false, 255},
            {"s2", "S2", PinType::PIN_DIGITAL, false, 255},
            {"en", "Enable", PinType::PIN_DIGITAL, true, 255}
        };
        
        return createCommonBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardMux")
            .setImplemented(IMPLEMENTED)
            .setAdditionalPins(pins, 4)
            .addFormField(makeNumberField("muxMin", "Seuil minimum", 0, 4095, "0", 1, 100, "f"))
            .addFormField(makeNumberField("muxMax", "Seuil maximum", 0, 4095, "4095", 1, 100, "f"))
            .addFormField(makeInfoField(
                "_muxCalibHint",
                "Adresses OSC pour le calibrage: /mux[ID]/cal/min [CH] /mux[ID]/cal/max [CH] /mux[ID]/cal/reset [CH] (CH=0-15)"
            ))
            .addFormField(makeNumberFieldWithHint(
                "muxFilterIntensity",
                "Intensité filtrage (1-10)",
                1, 10, "5", "1=rapide, 10=stable", 60, "f"
            ))
            .build();
    }
};

} // namespace Components
