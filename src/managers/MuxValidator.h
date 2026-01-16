#pragma once

#include <Arduino.h>

// Forward declarations
class ComponentManager;

/**
 * @brief Validateur pour les multiplexeurs
 * 
 * Valide les pins GPIO, les seuils, et gère la suppression
 * des composants existants sur les pins utilisées par le MUX.
 */
class MuxValidator {
public:
    /**
     * @brief Structure pour le résultat de validation
     */
    struct ValidationResult {
        bool valid;
        String error_message;
    };
    
    /**
     * @brief Valider les pins GPIO d'un MUX
     * @param sig Pin SIG
     * @param s0 Pin S0
     * @param s1 Pin S1
     * @param s2 Pin S2
     * @param s3 Pin S3
     * @param en Pin EN (255 = non connectée)
     * @return Résultat de validation
     */
    static ValidationResult validatePins(uint8_t sig, uint8_t s0, uint8_t s1, uint8_t s2, uint8_t s3, uint8_t en);
    
    /**
     * @brief Valider les seuils analogiques
     * @param analog_min Seuil minimum
     * @param analog_max Seuil maximum
     * @return Résultat de validation
     */
    static ValidationResult validateThresholds(uint16_t analog_min, uint16_t analog_max);
    
    /**
     * @brief Supprimer les composants existants sur les pins du MUX
     * @param manager Référence au ComponentManager
     * @param sig Pin SIG
     * @param s0 Pin S0
     * @param s1 Pin S1
     * @param s2 Pin S2
     * @param s3 Pin S3
     * @param en Pin EN (255 = non connectée)
     */
    static void removeExistingComponents(
        ComponentManager& manager,
        uint8_t sig, uint8_t s0, uint8_t s1, uint8_t s2, uint8_t s3, uint8_t en
    );
    
    /**
     * @brief Normaliser les paramètres MIDI
     * @param cc_base Base CC (sera clampé à 0-127)
     * @param midi_channel Canal MIDI (sera clampé à 1-16)
     */
    static void normalizeMidiParams(uint8_t& cc_base, uint8_t& midi_channel);
};
