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
 * @brief Famille de composants (catégorie)
 * Chaque famille correspond à un dossier dans src/components/
 */
enum class ComponentFamily : uint8_t {
    BASIC = 0,        // Potentiomètre, Bouton, LED (composants simples)
    MULTIPLEXER = 1,  // HC4067, HC4051, CD4052...
    ENCODER = 2,      // Encodeurs rotatifs
    SCREEN = 3        // Écrans OLED, LCD (évite conflit avec macro DISPLAY d'Arduino)
    // Ajouter de nouvelles familles ici
};

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
    const char* statusTemplate;  // Template pour le texte de statut (ex: "CC#{cc}", "Note {note}")
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
 * @brief Nombre max de champs de formulaire par composant
 */
static constexpr uint8_t MAX_FORM_FIELDS = 20;

/**
 * @brief Type de champ de formulaire
 */
enum class FieldType : uint8_t {
    TEXT = 0,
    NUMBER = 1,
    SELECT = 2,
    CHECKBOX = 3,
    RANGE = 4,
    INFO = 5        // Pour texte informatif seul
};

/**
 * @brief Position du hint/commentaire
 */
enum class HintPosition : uint8_t {
    NONE = 0,
    INLINE = 1,     // Span inline après le champ
    BELOW = 2       // Div.hint en dessous
};

/**
 * @brief Description d'un champ de formulaire
 */
struct FormFieldDef {
    const char* id;              // ID unique du champ (ex: "ledMode", "rtpCc")
    const char* label;           // Label principal (null pour INFO)
    FieldType type;             // Type de champ
    bool required;              // Champ requis
    
    // Pour TEXT
    const char* placeholder;    // Placeholder
    uint16_t maxLength;         // Longueur max (0 = illimité)
    const char* pattern;        // Regex pour validation
    
    // Pour NUMBER/RANGE
    int min;                    // Valeur minimale
    int max;                    // Valeur maximale
    int step;                   // Incrément (défaut: 1)
    
    // Pour SELECT
    const char* options;        // JSON: [{"value":"v","label":"L"},...]
    
    // Pour RANGE
    const char* separator;      // Texte entre min/max (défaut: "→")
    
    // Valeur par défaut
    const char* defaultValue;   // String (pour tous types)
    
    // Hints et commentaires
    HintPosition hintPosition;  // Position du hint
    const char* hint;           // Texte du hint/commentaire
    const char* hintClass;      // Classe CSS additionnelle
    
    // Affichage conditionnel
    const char* dependsOn;      // ID du champ dont dépend l'affichage
    const char* showWhen;       // Valeur(s) qui affichent ce champ (JSON array ou string)
    
    // Style et layout
    const char* wrapperClass;   // Classe du wrapper (défaut: "r" ou "f")
    const char* inputClass;     // Classe additionnelle pour l'input
    uint16_t width;            // Largeur en px (0 = auto)
    
    // Labels additionnels (pour champs complexes)
    const char* labelBefore;    // Label avant le champ
    const char* labelAfter;     // Label après le champ
};

/**
 * @struct ComponentDefinition
 * @brief Métadonnées d'un type de composant
 */
struct ComponentDefinition {
    const char* id;              // Identifiant interne unique (ex: "potentiometer", "hc4067")
    const char* displayName;     // Nom affiché dans l'UI (ex: "Potentiomètre", "HC4067 (16 canaux)")
    const char* icon;            // Icône (optionnel, pour l'UI)
    const char* cardId;          // ID de la carte HTML (ex: "cardPot", "cardMux")
    ComponentFamily family;      // Famille (BASIC, MULTIPLEXER, ENCODER, DISPLAY)
    const char* familyName;      // Nom de la famille pour l'UI (ex: "Basic", "Multiplexeur")
    ComponentType type;          // Type enum correspondant
    PinType pinType;             // Type de pin principale
    bool implemented;            // true = disponible, false = grisé dans l'UI
    bool isComplex;              // true = nécessite un manager (ex: MUX)
    bool supportsMidi;           // true = peut envoyer/recevoir MIDI
    bool supportsOsc;            // true = peut envoyer/recevoir OSC
    const char* statusTextTemplate; // Template par défaut pour le texte de statut
    const char* statusValueMappings; // JSON: {"ledMode": {"pwm": "PWM", "onoff": "On/Off"}}
    
    // Champs de formulaire pour l'UI
    uint8_t formFieldCount;     // Nombre de champs de formulaire
    FormFieldDef formFields[MAX_FORM_FIELDS]; // Champs de formulaire
    
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
            "{\"id\":\"%s\",\"displayName\":\"%s\",\"cardId\":\"%s\",\"family\":%d,\"familyName\":\"%s\","
            "\"pinType\":%d,\"implemented\":%s,\"isComplex\":%s,"
            "\"supportsMidi\":%s,\"supportsOsc\":%s,\"additionalPinCount\":%d",
            id,
            displayName,
            cardId ? cardId : "",
            static_cast<int>(family),
            familyName ? familyName : "",
            static_cast<int>(pinType),
            implemented ? "true" : "false",
            isComplex ? "true" : "false",
            supportsMidi ? "true" : "false",
            supportsOsc ? "true" : "false",
            additionalPinCount
        );
        
        // Ajouter statusTextTemplate si présent
        if (statusTextTemplate && written < (int)bufferSize - 50) {
            written += snprintf(buffer + written, bufferSize - written,
                ",\"statusTextTemplate\":\"%s\"",
                statusTextTemplate
            );
        }
        
        // Ajouter statusValueMappings si présent
        if (statusValueMappings && written < (int)bufferSize - 50) {
            written += snprintf(buffer + written, bufferSize - written,
                ",\"statusValueMappings\":%s",
                statusValueMappings
            );
        }
        
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
                    "{\"id\":\"%s\",\"displayName\":\"%s\"",
                    midiMessages[i].id,
                    midiMessages[i].displayName
                );
                // Ajouter statusTemplate si présent
                if (midiMessages[i].statusTemplate && written < (int)bufferSize - 50) {
                    written += snprintf(buffer + written, bufferSize - written,
                        ",\"statusTemplate\":\"%s\"",
                        midiMessages[i].statusTemplate
                    );
                }
                written += snprintf(buffer + written, bufferSize - written, "}");
            }
            written += snprintf(buffer + written, bufferSize - written, "]");
        }
        
        // Ajouter les champs de formulaire
        if (formFieldCount > 0 && written < (int)bufferSize - 50) {
            written += snprintf(buffer + written, bufferSize - written, ",\"formFields\":[");
            for (uint8_t i = 0; i < formFieldCount && i < MAX_FORM_FIELDS; i++) {
                if (i > 0) {
                    written += snprintf(buffer + written, bufferSize - written, ",");
                }
                const FormFieldDef& field = formFields[i];
                written += snprintf(buffer + written, bufferSize - written,
                    "{\"id\":\"%s\",\"type\":%d",
                    field.id ? field.id : "",
                    static_cast<int>(field.type)
                );
                
                if (field.label) {
                    written += snprintf(buffer + written, bufferSize - written,
                        ",\"label\":\"%s\"",
                        field.label
                    );
                }
                
                if (field.required) {
                    written += snprintf(buffer + written, bufferSize - written, ",\"required\":true");
                }
                
                if (field.placeholder) {
                    written += snprintf(buffer + written, bufferSize - written,
                        ",\"placeholder\":\"%s\"",
                        field.placeholder
                    );
                }
                
                if (field.maxLength > 0) {
                    written += snprintf(buffer + written, bufferSize - written,
                        ",\"maxLength\":%d",
                        field.maxLength
                    );
                }
                
                if (field.pattern) {
                    written += snprintf(buffer + written, bufferSize - written,
                        ",\"pattern\":\"%s\"",
                        field.pattern
                    );
                }
                
                if (field.type == FieldType::NUMBER || field.type == FieldType::RANGE) {
                    written += snprintf(buffer + written, bufferSize - written,
                        ",\"min\":%d,\"max\":%d",
                        field.min,
                        field.max
                    );
                    if (field.step != 1) {
                        written += snprintf(buffer + written, bufferSize - written,
                            ",\"step\":%d",
                            field.step
                        );
                    }
                }
                
                if (field.type == FieldType::SELECT && field.options) {
                    written += snprintf(buffer + written, bufferSize - written,
                        ",\"options\":%s",
                        field.options
                    );
                }
                
                if (field.type == FieldType::RANGE && field.separator) {
                    written += snprintf(buffer + written, bufferSize - written,
                        ",\"separator\":\"%s\"",
                        field.separator
                    );
                }
                
                if (field.defaultValue) {
                    // Échapper les guillemets dans defaultValue si nécessaire
                    written += snprintf(buffer + written, bufferSize - written,
                        ",\"defaultValue\":\"%s\"",
                        field.defaultValue
                    );
                }
                
                if (field.hintPosition != HintPosition::NONE && field.hint) {
                    written += snprintf(buffer + written, bufferSize - written,
                        ",\"hintPosition\":%d,\"hint\":\"%s\"",
                        static_cast<int>(field.hintPosition),
                        field.hint
                    );
                    if (field.hintClass) {
                        written += snprintf(buffer + written, bufferSize - written,
                            ",\"hintClass\":\"%s\"",
                            field.hintClass
                        );
                    }
                }
                
                if (field.dependsOn) {
                    written += snprintf(buffer + written, bufferSize - written,
                        ",\"dependsOn\":\"%s\"",
                        field.dependsOn
                    );
                    if (field.showWhen) {
                        written += snprintf(buffer + written, bufferSize - written,
                            ",\"showWhen\":%s",
                            field.showWhen
                        );
                    }
                }
                
                if (field.wrapperClass) {
                    written += snprintf(buffer + written, bufferSize - written,
                        ",\"wrapperClass\":\"%s\"",
                        field.wrapperClass
                    );
                }
                
                if (field.inputClass) {
                    written += snprintf(buffer + written, bufferSize - written,
                        ",\"inputClass\":\"%s\"",
                        field.inputClass
                    );
                }
                
                if (field.width > 0) {
                    written += snprintf(buffer + written, bufferSize - written,
                        ",\"width\":%d",
                        field.width
                    );
                }
                
                if (field.labelBefore) {
                    written += snprintf(buffer + written, bufferSize - written,
                        ",\"labelBefore\":\"%s\"",
                        field.labelBefore
                    );
                }
                
                if (field.labelAfter) {
                    written += snprintf(buffer + written, bufferSize - written,
                        ",\"labelAfter\":\"%s\"",
                        field.labelAfter
                    );
                }
                
                written += snprintf(buffer + written, bufferSize - written, "}");
            }
            written += snprintf(buffer + written, bufferSize - written, "]");
        }
        
        written += snprintf(buffer + written, bufferSize - written, "}");
        return written;
    }
};
