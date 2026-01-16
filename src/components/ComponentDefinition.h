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
 * - Les pins utilisées (pour le grisage)
 * - Les messages MIDI supportés
 */

/**
 * @brief Description d'une pin additionnelle pour composants complexes
 */
struct AdditionalPinDef {
    const char* id;              // Identifiant (ex: "s0", "s1", "en")
    const char* displayName;     // Nom affiché (ex: "S0", "S1", "Enable")
    PinType pinType;             // Type de pin requis
    bool optional;               // true = optionnel (ex: EN pour MUX)
    uint8_t defaultValue;        // Valeur par défaut (255 = non connecté)
};

/**
 * @brief Description d'un type de message MIDI
 */
struct MidiMessageDef {
    const char* id;              // Identifiant (ex: "cc", "note", "pc")
    const char* displayName;     // Nom affiché (ex: "Control Change", "Note")
};

/**
 * @brief Nombre max de pins additionnelles par composant
 */
static constexpr uint8_t MAX_ADDITIONAL_PINS = 6;

/**
 * @brief Nombre max de types de messages MIDI par composant
 */
static constexpr uint8_t MAX_MIDI_MESSAGES = 8;

/**
 * @struct ComponentDefinition
 * @brief Métadonnées d'un type de composant
 */
struct ComponentDefinition {
    const char* id;              // Identifiant interne (ex: "potentiometer", "button")
    const char* displayName;     // Nom affiché dans l'UI (ex: "Potentiomètre", "Bouton")
    const char* icon;            // Icône (optionnel, pour l'UI)
    ComponentType type;          // Type enum correspondant
    PinType pinType;             // Type de pin principale
    bool implemented;            // true = disponible, false = grisé dans l'UI
    bool isComplex;              // true = nécessite un manager (ex: MUX)
    bool supportsMidi;           // true = peut envoyer/recevoir MIDI
    bool supportsOsc;            // true = peut envoyer/recevoir OSC
    
    // Pins additionnelles pour composants complexes
    uint8_t additionalPinCount;  // Nombre de pins additionnelles (0 pour simple)
    AdditionalPinDef additionalPins[MAX_ADDITIONAL_PINS]; // Description des pins
    
    // Messages MIDI supportés
    uint8_t midiMessageCount;    // Nombre de types de messages MIDI supportés
    MidiMessageDef midiMessages[MAX_MIDI_MESSAGES]; // Types de messages supportés
    
    /**
     * @brief Convertit la définition en JSON pour l'API
     * @param buffer Buffer de sortie
     * @param bufferSize Taille du buffer
     * @return Nombre de caractères écrits
     */
    int toJson(char* buffer, size_t bufferSize) const {
        int written = snprintf(buffer, bufferSize,
            "{\"id\":\"%s\",\"displayName\":\"%s\",\"pinType\":%d,\"implemented\":%s,\"isComplex\":%s,"
            "\"supportsMidi\":%s,\"supportsOsc\":%s,\"additionalPinCount\":%d",
            id,
            displayName,
            static_cast<int>(pinType),
            implemented ? "true" : "false",
            isComplex ? "true" : "false",
            supportsMidi ? "true" : "false",
            supportsOsc ? "true" : "false",
            additionalPinCount
        );
        
        // Ajouter les pins additionnelles si présentes
        if (additionalPinCount > 0 && written < (int)bufferSize - 50) {
            written += snprintf(buffer + written, bufferSize - written, ",\"additionalPins\":[");
            for (uint8_t i = 0; i < additionalPinCount && i < MAX_ADDITIONAL_PINS; i++) {
                if (i > 0) {
                    written += snprintf(buffer + written, bufferSize - written, ",");
                }
                written += snprintf(buffer + written, bufferSize - written,
                    "{\"id\":\"%s\",\"displayName\":\"%s\",\"pinType\":%d,\"optional\":%s}",
                    additionalPins[i].id,
                    additionalPins[i].displayName,
                    static_cast<int>(additionalPins[i].pinType),
                    additionalPins[i].optional ? "true" : "false"
                );
            }
            written += snprintf(buffer + written, bufferSize - written, "]");
        }
        
        // Ajouter les messages MIDI supportés
        if (midiMessageCount > 0 && written < (int)bufferSize - 50) {
            written += snprintf(buffer + written, bufferSize - written, ",\"midiMessages\":[");
            for (uint8_t i = 0; i < midiMessageCount && i < MAX_MIDI_MESSAGES; i++) {
                if (i > 0) {
                    written += snprintf(buffer + written, bufferSize - written, ",");
                }
                written += snprintf(buffer + written, bufferSize - written,
                    "{\"id\":\"%s\",\"displayName\":\"%s\"}",
                    midiMessages[i].id,
                    midiMessages[i].displayName
                );
            }
            written += snprintf(buffer + written, bufferSize - written, "]");
        }
        
        written += snprintf(buffer + written, bufferSize - written, "}");
        return written;
    }
};
