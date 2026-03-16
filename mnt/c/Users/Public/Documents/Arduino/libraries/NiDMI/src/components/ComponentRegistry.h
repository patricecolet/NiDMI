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
    
#ifdef NIDMI_COMPONENT_DEFS_PAGINATION
    /**
     * @brief Convertit les définitions en JSON array avec pagination
     * @param buffer Buffer de sortie
     * @param bufferSize Taille du buffer
     * @param page Numéro de page (0-based)
     * @param limit Nombre d'éléments par page
     * @return Nombre de caractères écrits
     */
    static int toJsonArrayPage(char* buffer, size_t bufferSize, int page, int limit);
#endif
    
    /**
     * @brief Obtient le nombre de composants enregistrés
     */
    static size_t count();
    
    /**
     * @brief Libère la mémoire allouée pour toutes les définitions
     * À appeler lors du cleanup (optionnel sur ESP32 car le programme ne se termine jamais)
     */
    static void cleanup();

    /**
     * @brief Enregistre une définition de composant (appelé automatiquement au chargement)
     * @param def Définition à enregistrer (sera déplacée, ne pas utiliser après l'appel)
     * @return true si l'enregistrement a réussi
     * 
     * Cette méthode est appelée automatiquement par les variables statiques
     * dans les fichiers .cpp des définitions lors du chargement du module.
     * Les définitions sont stockées temporairement et déplacées dans le
     * vecteur principal lors de init().
     */
    static bool registerDefinition(ComponentDefinition&& def);

private:
    static std::vector<ComponentDefinition>* definitions_ptr;  // Pointeur au lieu d'objet statique (évite crash au boot sur ESP32-C3)
    static std::vector<ComponentDefinition>* pending_definitions_ptr;  // Vecteur temporaire pour l'enregistrement automatique
    static bool initialized_;
};
