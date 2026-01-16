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
 * @brief Constantes pour le Multiplexeur
 */
struct Mux {
    // Identifiants
    static constexpr const char* ID = "mux";
    static constexpr const char* DISPLAY_NAME = "Multiplexeur";
    static constexpr const char* TYPE_HC4067 = "HC4067";
    
    // Configuration
    static constexpr ComponentType TYPE = ComponentType::MUX;
    static constexpr PinType PIN_TYPE = PinType::PIN_ANALOG;  // Pour la pin SIG
    static constexpr bool IMPLEMENTED = true;
    static constexpr bool IS_COMPLEX = true;  // Nécessite MuxManager
    
    // Caractéristiques
    static constexpr uint8_t NUM_CHANNELS = 16;
    static constexpr uint8_t NUM_ADDRESS_PINS = 4;  // S0-S3
    static constexpr uint8_t NO_PIN = 255;          // Valeur pour EN non connecté
    
    // Valeurs par défaut
    static constexpr uint8_t DEFAULT_CC_BASE = 1;
    static constexpr uint8_t DEFAULT_CHANNEL = 1;
    static constexpr uint8_t DEFAULT_FILTER_INTENSITY = 5;
    static constexpr uint16_t DEFAULT_ANALOG_MIN = 0;
    static constexpr uint16_t DEFAULT_ANALOG_MAX = 4095;
    
    /**
     * @brief Validation basique (SIG pin seulement)
     * Pour la validation complète, utiliser MuxValidator
     * @param gpio GPIO de la pin SIG
     * @return true si le GPIO a une capacité ADC
     */
    static bool validateSigPin(uint8_t gpio);
    
    /**
     * @brief Crée la définition pour le registre
     */
    static ComponentDefinition createDefinition() {
        return {
            ID,
            DISPLAY_NAME,
            nullptr,
            TYPE,
            PIN_TYPE,
            IMPLEMENTED,
            IS_COMPLEX
        };
    }
};

} // namespace Components
