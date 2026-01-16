#pragma once

#include "ComponentTypes.h"

/**
 * @file ComponentDefinition.h
 * @brief Structure de définition d'un composant pour l'UI et la validation
 * 
 * Chaque composant (potentiomètre, bouton, LED, MUX, etc.) a une définition
 * qui décrit ses caractéristiques pour :
 * - L'UI frontend (nom affiché, icône, formulaire)
 * - La validation (type de pin requis, fonction de validation)
 * - L'état d'implémentation (composant disponible ou grisé)
 */

/**
 * @struct ComponentDefinition
 * @brief Métadonnées d'un type de composant
 */
struct ComponentDefinition {
    const char* id;              // Identifiant interne (ex: "potentiometer", "button")
    const char* displayName;     // Nom affiché dans l'UI (ex: "Potentiomètre", "Bouton")
    const char* icon;            // Icône (optionnel, pour l'UI)
    ComponentType type;          // Type enum correspondant
    PinType pinType;             // Type de pin requis
    bool implemented;            // true = disponible, false = grisé dans l'UI
    bool isComplex;              // true = nécessite un manager (ex: MUX)
    
    /**
     * @brief Convertit la définition en JSON pour l'API
     * @param buffer Buffer de sortie
     * @param bufferSize Taille du buffer
     * @return Nombre de caractères écrits
     */
    int toJson(char* buffer, size_t bufferSize) const {
        return snprintf(buffer, bufferSize,
            "{\"id\":\"%s\",\"displayName\":\"%s\",\"pinType\":%d,\"implemented\":%s,\"isComplex\":%s}",
            id,
            displayName,
            static_cast<int>(pinType),
            implemented ? "true" : "false",
            isComplex ? "true" : "false"
        );
    }
};
