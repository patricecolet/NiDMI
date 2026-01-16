#pragma once

#include "ComponentTypes.h"
#include <cstring>

/**
 * @file ValidationRegistry.h
 * @brief Registre centralisé des fonctions de validation par type de composant
 * 
 * Ce registre permet d'enregistrer des fonctions de validation pour chaque
 * type de composant. Les composants simples ont une validation inline,
 * les composants complexes peuvent utiliser des validators externes.
 * 
 * Usage:
 *   ValidationRegistry::registerValidator("potentiometer", 
 *       [](uint8_t gpio, const void*) { return PinMapper::hasAdc(gpio); });
 *   
 *   bool valid = ValidationRegistry::validate("potentiometer", gpio);
 */

/**
 * @brief Signature d'une fonction de validation
 * @param gpio Le GPIO à valider
 * @param config Configuration optionnelle (pour composants complexes)
 * @return true si la configuration est valide
 */
using ValidatorFunc = bool (*)(uint8_t gpio, const void* config);

/**
 * @brief Structure pour stocker un validator
 */
struct ValidatorEntry {
    const char* componentId;  // Pointeur vers chaîne statique (pas de std::string)
    ValidatorFunc validator;
};

class ValidationRegistry {
public:
    /**
     * @brief Enregistre un validator pour un type de composant
     * @param componentId Identifiant du composant (ex: "potentiometer") - doit être une chaîne statique
     * @param validator Fonction de validation (pointeur de fonction, pas std::function)
     */
    static void registerValidator(const char* componentId, ValidatorFunc validator);
    
    /**
     * @brief Valide une configuration
     * @param componentId Identifiant du composant
     * @param gpio GPIO à valider
     * @param config Configuration optionnelle (nullptr pour composants simples)
     * @return true si valide, false si invalide ou pas de validator enregistré
     */
    static bool validate(const char* componentId, uint8_t gpio, const void* config = nullptr);
    
    /**
     * @brief Vérifie si un validator existe pour un composant
     * @param componentId Identifiant du composant
     * @return true si un validator est enregistré
     */
    static bool hasValidator(const char* componentId);
    
    /**
     * @brief Initialise les validators par défaut
     * Appelé au démarrage pour enregistrer tous les validators
     */
    static void init();

private:
    static constexpr size_t MAX_VALIDATORS = 10;
    static ValidatorEntry validators_[MAX_VALIDATORS];
    static size_t validatorCount_;
};
