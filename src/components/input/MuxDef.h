#pragma once

#include "../ComponentDefinition.h"

/**
 * @file MuxDef.h
 * @brief Définition du composant Multiplexeur Analogique
 * 
 * Composant d'entrée complexe.
 * Gère un multiplexeur 16 canaux (HC4067) pour lire plusieurs potentiomètres
 * via une seule pin analogique.
 * 
 * Pins requises:
 * - 1 pin analogique (SIG)
 * - 4 pins digitales (S0, S1, S2, S3)
 * - 1 pin digitale optionnelle (EN)
 * 
 * Ce composant est "complexe" car il nécessite MuxManager pour la gestion.
 */

namespace Components {

/**
 * @brief Définition complète du Multiplexeur
 */
struct Mux {
    // Identifiants
    static constexpr const char* ID = "mux";
    static constexpr const char* DISPLAY_NAME = "Multiplexeur";
    static constexpr const char* TYPE_HC4067 = "HC4067";
    
    // Configuration
    static constexpr ComponentType TYPE = ComponentType::MUX;
    static constexpr PinType PIN_TYPE = PinType::PIN_ANALOG;
    static constexpr bool IMPLEMENTED = true;
    static constexpr bool IS_COMPLEX = true;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC = true;
    
    // Caractéristiques
    static constexpr uint8_t NUM_CHANNELS = 16;
    static constexpr uint8_t NUM_ADDRESS_PINS = 4;
    static constexpr uint8_t NO_PIN = 255;
    
    // Valeurs par défaut
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
     * @brief Crée la définition complète pour le registre
     */
    static ComponentDefinition createDefinition() {
        ComponentDefinition def = {};
        def.id = ID;
        def.displayName = DISPLAY_NAME;
        def.icon = nullptr;
        def.type = TYPE;
        def.pinType = PIN_TYPE;
        def.implemented = IMPLEMENTED;
        def.isComplex = IS_COMPLEX;
        def.supportsMidi = SUPPORTS_MIDI;
        def.supportsOsc = SUPPORTS_OSC;
        
        // Pins additionnelles (S0, S1, S2, S3, EN)
        def.additionalPinCount = 5;
        def.additionalPins[0] = {"s0", "S0", PinType::PIN_DIGITAL, false, 255};
        def.additionalPins[1] = {"s1", "S1", PinType::PIN_DIGITAL, false, 255};
        def.additionalPins[2] = {"s2", "S2", PinType::PIN_DIGITAL, false, 255};
        def.additionalPins[3] = {"s3", "S3", PinType::PIN_DIGITAL, false, 255};
        def.additionalPins[4] = {"en", "Enable", PinType::PIN_DIGITAL, true, 255};
        
        // Messages MIDI supportés (même que potentiomètre)
        def.midiMessageCount = 4;
        def.midiMessages[0] = {"cc", "Control Change"};
        def.midiMessages[1] = {"pc", "Program Change"};
        def.midiMessages[2] = {"pitchbend", "Pitch Bend"};
        def.midiMessages[3] = {"aftertouch", "Aftertouch (Channel)"};
        
        return def;
    }
};

} // namespace Components
