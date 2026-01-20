#pragma once

#include "ComponentTypes.h"
#include <cstring>
#include <Arduino.h>

// Forward declaration (défini dans ComplexHandler.h)
struct ComplexComponentData;

/**
 * @file ValidationRegistry.h
 * @brief Registre centralisé des fonctions de validation par type de composant
 * 
 * Ce registre permet d'enregistrer des fonctions de validation pour chaque
 * type de composant. Les composants simples ont une validation inline,
 * les composants complexes peuvent utiliser des validators externes.
 * 
 * Usage composants simples:
 *   ValidationRegistry::registerValidator("potentiometer", 
 *       [](uint8_t gpio, const void*) { return PinMapper::hasAdc(gpio); });
 *   
 *   bool valid = ValidationRegistry::validate("potentiometer", gpio);
 * 
 * Usage composants complexes:
 *   ValidationRegistry::registerComplexValidator("hc4067", validateMuxComplex);
 *   
 *   ValidationResult result = ValidationRegistry::validateComplex("hc4067", data);
 */

/**
 * @brief Structure pour le résultat de validation (avec message d'erreur)
 */
struct ValidationResult {
    bool valid;
    String error_message;
    
    ValidationResult() : valid(true), error_message("") {}
    ValidationResult(bool v, const String& msg = "") : valid(v), error_message(msg) {}
};

/**
 * @brief Signature d'une fonction de validation pour composants simples
 * @param gpio Le GPIO à valider
 * @param config Configuration optionnelle (pour composants complexes)
 * @return true si la configuration est valide
 */
using ValidatorFunc = bool (*)(uint8_t gpio, const void* config);

/**
 * @brief Signature d'une fonction de validation pour composants complexes
 * @param data Données complètes du composant complexe
 * @return ValidationResult avec valid=true si valide, false + message d'erreur sinon
 */
using ComplexValidatorFunc = ValidationResult (*)(const ComplexComponentData& data);

/**
 * @brief Structure pour stocker un validator simple
 */
struct ValidatorEntry {
    const char* componentId;  // Pointeur vers chaîne statique (pas de std::string)
    ValidatorFunc validator;
};

/**
 * @brief Structure pour stocker un validator complexe
 */
struct ComplexValidatorEntry {
    const char* componentId;  // Pointeur vers chaîne statique (pas de std::string)
    ComplexValidatorFunc validator;
};

class ValidationRegistry {
public:
    /**
     * @brief Enregistre un validator pour un type de composant simple
     * @param componentId Identifiant du composant (ex: "potentiometer") - doit être une chaîne statique
     * @param validator Fonction de validation (pointeur de fonction, pas std::function)
     */
    static void registerValidator(const char* componentId, ValidatorFunc validator);
    
    /**
     * @brief Valide une configuration pour un composant simple
     * @param componentId Identifiant du composant
     * @param gpio GPIO à valider
     * @param config Configuration optionnelle (nullptr pour composants simples)
     * @return true si valide, false si invalide ou pas de validator enregistré
     */
    static bool validate(const char* componentId, uint8_t gpio, const void* config = nullptr);
    
    /**
     * @brief Enregistre un validator pour un type de composant complexe
     * @param componentId Identifiant du composant (ex: "hc4067") - doit être une chaîne statique
     * @param validator Fonction de validation complexe
     * @return true si l'enregistrement a réussi
     */
    static bool registerComplexValidator(const char* componentId, ComplexValidatorFunc validator);
    
    /**
     * @brief Valide une configuration pour un composant complexe
     * @param componentId Identifiant du composant
     * @param data Données complètes du composant complexe
     * @return ValidationResult avec valid=true si valide, false + message d'erreur sinon
     */
    static ValidationResult validateComplex(const char* componentId, const ComplexComponentData& data);
    
    /**
     * @brief Vérifie si un validator existe pour un composant simple
     * @param componentId Identifiant du composant
     * @return true si un validator est enregistré
     */
    static bool hasValidator(const char* componentId);
    
    /**
     * @brief Vérifie si un validator complexe existe pour un composant
     * @param componentId Identifiant du composant
     * @return true si un validator complexe est enregistré
     */
    static bool hasComplexValidator(const char* componentId);
    
    /**
     * @brief Initialise les validators par défaut
     * Appelé au démarrage pour enregistrer tous les validators
     */
    static void init();

private:
    static constexpr size_t MAX_VALIDATORS = 10;
    static constexpr size_t MAX_COMPLEX_VALIDATORS = 10;
    static ValidatorEntry validators_[MAX_VALIDATORS];
    static size_t validatorCount_;
    static ComplexValidatorEntry complexValidators_[MAX_COMPLEX_VALIDATORS];
    static size_t complexValidatorCount_;
};
