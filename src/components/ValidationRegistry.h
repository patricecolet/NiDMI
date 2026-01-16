#pragma once

#include "ComponentTypes.h"
#include <functional>
#include <map>
#include <string>

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

class ValidationRegistry {
public:
    /**
     * @brief Signature d'une fonction de validation
     * @param gpio Le GPIO à valider
     * @param config Configuration optionnelle (pour composants complexes)
     * @return true si la configuration est valide
     */
    using ValidatorFunc = std::function<bool(uint8_t gpio, const void* config)>;
    
    /**
     * @brief Enregistre un validator pour un type de composant
     * @param componentId Identifiant du composant (ex: "potentiometer")
     * @param validator Fonction de validation
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
    static std::map<std::string, ValidatorFunc> validators_;
};
