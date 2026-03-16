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
    BASIC       = 0,  // Potentiomètre, Bouton, LED (composants simples)
    MULTIPLEXER = 1,  // HC4067, HC4051, CD4052...
    ENCODER     = 2,  // Encodeurs rotatifs
    SCREEN      = 3,  // Écrans OLED, LCD (évite conflit avec macro DISPLAY d'Arduino)

    // Nouvelles familles fonctionnelles pour capteurs/actuateurs Seeed/Grove
    DISTANCE    = 4,  // Ultrasonic, LiDAR, IR distance, inductif
    ENVIRONMENT = 5,  // Température, humidité, pression, lumière, UV, sol
    MOTION      = 6,  // PIR, IMU, radar, gestes
    COLOR       = 7,  // Capteurs de couleur / lumière avancés
    INTERFACE   = 8,  // Touch/MPR121, FSR, encodeurs, contrôles physiques
    ACTUATOR    = 9   // Relais, moteurs, servos, buzzers
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
 * @brief Nombre max de paramètres MIDI par type de message
 */
static constexpr uint8_t MAX_MIDI_PARAMS = 10;

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
 * @brief Description d'un paramètre MIDI
 */
struct MidiParamDef {
    const char* id;              // ID du paramètre (ex: "rtpNote", "rtpCc", "rtpChan")
    const char* label;           // Label affiché
    FieldType type;             // Type de champ (NUMBER, RANGE, INFO)
    int min;                    // Valeur minimale (pour NUMBER/RANGE)
    int max;                    // Valeur maximale (pour NUMBER/RANGE)
    const char* placeholder;    // Placeholder (pour NUMBER)
    const char* defaultValue;   // Valeur par défaut (string)
    const char* defaultMin;     // Valeur min par défaut (pour RANGE)
    const char* defaultMax;      // Valeur max par défaut (pour RANGE)
    const char* separator;       // Séparateur (pour RANGE, défaut: "→")
    const char* hint;            // Hint/commentaire (pour INFO)
    const char* hintClass;      // Classe CSS pour hint
    uint16_t width;             // Largeur en px (0 = auto)
    const char* dependsOnRole;   // JSON array de rôles qui affichent ce paramètre (null = tous)
    
    // Constructeur explicite pour permettre l'initialisation par accolades
    MidiParamDef(const char* id = nullptr, const char* label = nullptr, FieldType type = FieldType::NUMBER,
                 int min = 0, int max = 0,
                 const char* placeholder = nullptr, const char* defaultValue = nullptr,
                 const char* defaultMin = nullptr, const char* defaultMax = nullptr,
                 const char* separator = nullptr,
                 const char* hint = nullptr, const char* hintClass = nullptr,
                 uint16_t width = 0, const char* dependsOnRole = nullptr)
        : id(id), label(label), type(type), min(min), max(max),
          placeholder(placeholder), defaultValue(defaultValue),
          defaultMin(defaultMin), defaultMax(defaultMax), separator(separator),
          hint(hint), hintClass(hintClass), width(width), dependsOnRole(dependsOnRole) {}
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
 * @brief Description d'un type de message MIDI
 */
struct MidiMessageDef {
    const char* id;              // Identifiant (ex: "cc", "note", "pc")
    const char* displayName;     // Nom affiché (ex: "Control Change", "Note")
    const char* statusTemplate;  // Template pour le texte de statut (ex: "CC#{cc}", "Note {note}")
    uint8_t paramCount;          // Nombre de paramètres requis pour ce message
    MidiParamDef* params;        // Pointeur vers heap (alloué avec new[])
    size_t paramsCapacity;       // Taille allouée (pour vérification)
    
    // Constructeur par défaut
    MidiMessageDef() : params(nullptr), paramsCapacity(0), paramCount(0) {}
    
    // Cleanup : libère la mémoire allouée
    void cleanup() {
        if (params) {
            delete[] params;
            params = nullptr;
            paramsCapacity = 0;
        }
    }
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
    // Note: isComplex supprimé - utiliser additionalPinCount > 0 pour détecter un composant avec pins additionnelles
    bool supportsMidi;           // true = peut envoyer/recevoir MIDI
    bool supportsOsc;            // true = peut envoyer/recevoir OSC
    const char* statusTextTemplate; // Template par défaut pour le texte de statut
    const char* statusValueMappings; // JSON: {"ledMode": {"pwm": "PWM", "onoff": "On/Off"}}
    
    // Champs de formulaire pour l'UI
    uint8_t formFieldCount;     // Nombre de champs de formulaire
    FormFieldDef* formFields;   // Pointeur vers heap (alloué avec new[])
    size_t formFieldsCapacity;  // Taille allouée (pour vérification)
    
    // Pins additionnelles pour composants complexes
    uint8_t additionalPinCount;  // Nombre de pins additionnelles (0 pour simple)
    AdditionalPinDef* additionalPins; // Pointeur vers heap (alloué avec new[])
    size_t additionalPinsCapacity;    // Taille allouée (pour vérification)
    
    // Messages MIDI supportés
    uint8_t midiMessageCount;    // Nombre de types de messages MIDI supportés
    MidiMessageDef* midiMessages; // Pointeur vers heap (alloué avec new[])
    size_t midiMessagesCapacity;  // Taille allouée (pour vérification)
    
    // Constructeur par défaut
    ComponentDefinition() : 
        id(nullptr), displayName(nullptr), icon(nullptr), cardId(nullptr),
        family(ComponentFamily::BASIC), familyName(nullptr),
        type(ComponentType::POTENTIOMETER), pinType(PinType::PIN_DIGITAL),
        implemented(false), supportsMidi(false), supportsOsc(false),
        statusTextTemplate(nullptr), statusValueMappings(nullptr),
        formFieldCount(0), formFields(nullptr), formFieldsCapacity(0),
        additionalPinCount(0), additionalPins(nullptr), additionalPinsCapacity(0),
        midiMessageCount(0), midiMessages(nullptr), midiMessagesCapacity(0) {}
    
    // Cleanup : libère la mémoire allouée (récursif)
    void cleanup() {
        if (formFields) {
            delete[] formFields;
            formFields = nullptr;
            formFieldsCapacity = 0;
        }
        if (additionalPins) {
            delete[] additionalPins;
            additionalPins = nullptr;
            additionalPinsCapacity = 0;
        }
        if (midiMessages) {
            // Libérer les params de chaque message
            for (size_t i = 0; i < midiMessagesCapacity; i++) {
                midiMessages[i].cleanup();
            }
            delete[] midiMessages;
            midiMessages = nullptr;
            midiMessagesCapacity = 0;
        }
    }
    
    /**
     * @brief Convertit la définition en JSON pour l'API
     * @param buffer Buffer de sortie
     * @param bufferSize Taille du buffer
     * @return Nombre de caractères écrits
     */
    int toJson(char* buffer, size_t bufferSize) const {
        // Protection contre buffer null ou trop petit
        if (!buffer || bufferSize < 100) return 0;
        
        int written = snprintf(buffer, bufferSize,
            "{\"id\":\"%s\",\"displayName\":\"%s\",\"cardId\":\"%s\",\"family\":%d,\"familyName\":\"%s\","
            "\"pinType\":%d,\"implemented\":%s,"
            "\"supportsMidi\":%s,\"supportsOsc\":%s,\"additionalPinCount\":%d",
            id,
            displayName,
            cardId ? cardId : "",
            static_cast<int>(family),
            familyName ? familyName : "",
            static_cast<int>(pinType),
            implemented ? "true" : "false",
            supportsMidi ? "true" : "false",
            supportsOsc ? "true" : "false",
            additionalPinCount
        );
        
        // Vérifier que le premier snprintf a réussi
        if (written < 0 || written >= (int)bufferSize) return 0;
        
        // Ajouter statusTextTemplate si présent
        if (statusTextTemplate && written < (int)bufferSize - 50) {
            int added = snprintf(buffer + written, bufferSize - written,
                ",\"statusTextTemplate\":\"%s\"",
                statusTextTemplate
            );
            if (added < 0 || written + added >= (int)bufferSize) return 0;
            written += added;
        }
        
        // Ajouter statusValueMappings si présent (omis en mode LIGHT)
#ifndef NIDMI_COMPONENT_DEFS_LIGHT
        if (statusValueMappings && written < (int)bufferSize - 50) {
            int added = snprintf(buffer + written, bufferSize - written,
                ",\"statusValueMappings\":%s",
                statusValueMappings
            );
            if (added < 0 || written + added >= (int)bufferSize) return 0;
            written += added;
        }
#endif
        
        // Ajouter les pins additionnelles si présentes
        if (additionalPinCount > 0 && additionalPins && written < (int)bufferSize - 50) {
            written += snprintf(buffer + written, bufferSize - written, ",\"additionalPins\":[");
            for (uint8_t i = 0; i < additionalPinCount && i < additionalPinsCapacity; i++) {
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
        if (midiMessageCount > 0 && midiMessages && written < (int)bufferSize - 50) {
            written += snprintf(buffer + written, bufferSize - written, ",\"midiMessages\":[");
            for (uint8_t i = 0; i < midiMessageCount && i < midiMessagesCapacity; i++) {
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
                // Ajouter les paramètres MIDI
                if (midiMessages[i].paramCount > 0 && midiMessages[i].params && written < (int)bufferSize - 50) {
                    written += snprintf(buffer + written, bufferSize - written, ",\"params\":[");
                    for (uint8_t j = 0; j < midiMessages[i].paramCount && j < midiMessages[i].paramsCapacity; j++) {
                        if (j > 0) {
                            written += snprintf(buffer + written, bufferSize - written, ",");
                        }
                        const MidiParamDef& param = midiMessages[i].params[j];
                        written += snprintf(buffer + written, bufferSize - written,
                            "{\"id\":\"%s\",\"type\":%d",
                            param.id ? param.id : "",
                            static_cast<int>(param.type)
                        );
                        
                        if (param.label) {
                            written += snprintf(buffer + written, bufferSize - written,
                                ",\"label\":\"%s\"",
                                param.label
                            );
                        }
                        
                        if (param.type == FieldType::NUMBER || param.type == FieldType::RANGE) {
                            written += snprintf(buffer + written, bufferSize - written,
                                ",\"min\":%d,\"max\":%d",
                                param.min,
                                param.max
                            );
                        }
                        
                        if (param.placeholder) {
                            written += snprintf(buffer + written, bufferSize - written,
                                ",\"placeholder\":\"%s\"",
                                param.placeholder
                            );
                        }
                        
                        if (param.defaultValue) {
                            written += snprintf(buffer + written, bufferSize - written,
                                ",\"defaultValue\":\"%s\"",
                                param.defaultValue
                            );
                        }
                        
                        if (param.type == FieldType::RANGE) {
                            if (param.defaultMin) {
                                written += snprintf(buffer + written, bufferSize - written,
                                    ",\"defaultMin\":\"%s\"",
                                    param.defaultMin
                                );
                            }
                            if (param.defaultMax) {
                                written += snprintf(buffer + written, bufferSize - written,
                                    ",\"defaultMax\":\"%s\"",
                                    param.defaultMax
                                );
                            }
                            if (param.separator) {
                                written += snprintf(buffer + written, bufferSize - written,
                                    ",\"separator\":\"%s\"",
                                    param.separator
                                );
                            }
                        }
                        
                        // Hint pour INFO (omis en mode LIGHT)
#ifndef NIDMI_COMPONENT_DEFS_LIGHT
                        if (param.type == FieldType::INFO && param.hint) {
                            written += snprintf(buffer + written, bufferSize - written,
                                ",\"hint\":\"%s\"",
                                param.hint
                            );
                            if (param.hintClass) {
                                written += snprintf(buffer + written, bufferSize - written,
                                    ",\"hintClass\":\"%s\"",
                                    param.hintClass
                                );
                            }
                        }
#endif
                        
                        if (param.dependsOnRole) {
                            written += snprintf(buffer + written, bufferSize - written,
                                ",\"dependsOnRole\":%s",
                                param.dependsOnRole
                            );
                        }
                        
                        if (param.width > 0) {
                            written += snprintf(buffer + written, bufferSize - written,
                                ",\"width\":%d",
                                param.width
                            );
                        }
                        
                        written += snprintf(buffer + written, bufferSize - written, "}");
                    }
                    written += snprintf(buffer + written, bufferSize - written, "]");
                }
                written += snprintf(buffer + written, bufferSize - written, "}");
            }
            written += snprintf(buffer + written, bufferSize - written, "]");
        }
        
        // Ajouter les champs de formulaire
        if (formFieldCount > 0 && formFields && written < (int)bufferSize - 50) {
            written += snprintf(buffer + written, bufferSize - written, ",\"formFields\":[");
            for (uint8_t i = 0; i < formFieldCount && i < formFieldsCapacity; i++) {
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
                    // field.options est déjà du JSON valide (ex: [{"value":"v","label":"L"}])
                    // L'insérer directement sans guillemets autour (pas de %s avec guillemets)
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
                
                // Hints pour formFields (omis en mode LIGHT)
#ifndef NIDMI_COMPONENT_DEFS_LIGHT
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
#endif
                
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
