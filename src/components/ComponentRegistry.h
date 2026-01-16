#pragma once

#include "ComponentDefinition.h"
#include "ValidationRegistry.h"
#include <vector>

/**
 * @file ComponentRegistry.h
 * @brief Registre centralisé de tous les composants disponibles
 * 
 * Ce registre contient les définitions de tous les composants (potentiomètre,
 * bouton, LED, MUX, etc.) et permet de les exposer au frontend via l'API.
 * 
 * Usage:
 *   ComponentRegistry::init();  // Au démarrage
 *   const auto& components = ComponentRegistry::getAll();
 *   ComponentRegistry::toJsonArray(buffer, bufferSize);
 */

class ComponentRegistry {
public:
    /**
     * @brief Initialise le registre avec tous les composants
     * Appelé une fois au démarrage
     */
    static void init();
    
    /**
     * @brief Obtient toutes les définitions de composants
     * @return Vecteur des définitions
     */
    static const std::vector<ComponentDefinition>& getAll();
    
    /**
     * @brief Trouve une définition par son ID
     * @param id Identifiant du composant (ex: "potentiometer")
     * @return Pointeur vers la définition, nullptr si non trouvé
     */
    static const ComponentDefinition* findById(const char* id);
    
    /**
     * @brief Trouve une définition par son ComponentType
     * @param type Type enum du composant
     * @return Pointeur vers la définition, nullptr si non trouvé
     */
    static const ComponentDefinition* findByType(ComponentType type);
    
    /**
     * @brief Convertit toutes les définitions en JSON array
     * @param buffer Buffer de sortie
     * @param bufferSize Taille du buffer
     * @return Nombre de caractères écrits
     */
    static int toJsonArray(char* buffer, size_t bufferSize);
    
    /**
     * @brief Obtient le nombre de composants enregistrés
     */
    static size_t count();
    
    /**
     * @brief Libère la mémoire allouée pour toutes les définitions
     * À appeler lors du cleanup (optionnel sur ESP32 car le programme ne se termine jamais)
     */
    static void cleanup();

private:
    static std::vector<ComponentDefinition> definitions_;
    static bool initialized_;
};
