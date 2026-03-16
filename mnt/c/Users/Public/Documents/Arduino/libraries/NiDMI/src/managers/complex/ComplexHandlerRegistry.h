#pragma once

#include <Arduino.h>
#include "ComplexHandler.h"

/**
 * @file ComplexHandlerRegistry.h
 * @brief Registre d'enregistrement automatique des handlers pour composants complexes
 * 
 * Permet d'enregistrer automatiquement des handlers pour chaque type de composant
 * avec additionalPins, similaire à ProcessorRegistry pour les composants simples.
 * 
 * Usage pour ajouter un nouveau composant :
 * ```cpp
 * // Dans MyComponentHandler.cpp
 * #include "ComplexHandlerRegistry.h"
 * #include "MyComponentHandler.h"
 * 
 * // Enregistrement automatique au chargement
 * static bool registered = ComplexHandlerRegistry::registerHandler(
 *     "mycomponent",
 *     []() -> ComplexHandler* { return new MyComponentHandler(); }
 * );
 * ```
 */

/**
 * @brief Type de fonction factory pour créer un handler
 */
typedef ComplexHandler* (*ComplexHandlerFactory)();

class ComplexHandlerRegistry {
private:
    struct HandlerEntry {
        const char* componentId;
        ComplexHandlerFactory factory;
        ComplexHandler* instance;  // Instance créée à la demande (singleton par type)
    };
    
    static constexpr uint8_t MAX_HANDLERS = 32;
    static HandlerEntry handlers[MAX_HANDLERS];
    static uint8_t handler_count;
    static bool initialized;

public:
    /**
     * @brief Enregistrer un handler pour un type de composant
     * @param componentId ID du composant (ex: "hc4067", "hc4051")
     * @param factory Fonction factory pour créer le handler
     * @return true si l'enregistrement a réussi
     */
    static bool registerHandler(const char* componentId, ComplexHandlerFactory factory);
    
    /**
     * @brief Obtenir le handler pour un type de composant
     * @param componentId ID du composant (ex: "hc4067")
     * @return Pointeur vers le handler, ou nullptr si non trouvé
     */
    static ComplexHandler* getHandler(const char* componentId);
    
    /**
     * @brief Vérifier si un handler est enregistré pour un type de composant
     * @param componentId ID du composant
     * @return true si un handler est enregistré
     */
    static bool hasHandler(const char* componentId);
    
    /**
     * @brief Initialiser tous les handlers enregistrés
     * Doit être appelé après tous les includes de handlers
     */
    static void init();
    
    /**
     * @brief Retourne le handler à un index donné (pour itération)
     */
    static ComplexHandler* getHandlerByIndex(uint8_t index);
    
    /**
     * @brief Retourne le nombre de handlers enregistrés
     */
    static uint8_t getHandlerCount() { return handler_count; }
};
