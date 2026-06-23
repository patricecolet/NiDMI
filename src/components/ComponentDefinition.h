#pragma once

#include "ComponentTypes.h"
#include <cstdarg>

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
    ACTUATOR    = 9,  // Relais, moteurs, servos, buzzers
    SIGNAL      = 10  // Modules signal/CV : bruit, LFO, oscillateurs, VCA (génération + échantillonnage)
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
    
    // Constructeur explicite pour permettre l'initialisation par accolades.
    // constexpr : permet de placer des tableaux de MidiParamDef en flash (.rodata)
    // via `static constexpr MidiParamDef[]` plutôt qu'en heap (new[]).
    constexpr MidiParamDef(const char* id = nullptr, const char* label = nullptr, FieldType type = FieldType::NUMBER,
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
    // Agrégat (pas de constructeur) avec initialiseurs par défaut : permet à la fois
    //   - `static constexpr MidiMessageDef[] = {{...}}` en flash (init agrégée),
    //   - `MidiMessageDef msg;` + `new MidiMessageDef[]` côté builder (value-init).
    // Ordre des champs == ordre d'init agrégée (voir MidiMessageCatalog.h).
    const char* id = nullptr;            // Identifiant (ex: "cc", "note", "pc")
    const char* displayName = nullptr;   // Nom affiché (ex: "Control Change", "Note")
    const char* statusTemplate = nullptr;// Template pour le texte de statut (ex: "CC#{cc}", "Note {note}")
    const char* axis = nullptr;          // Axe pour joystick ("x", "y", ou nullptr pour les deux)
    uint8_t paramCount = 0;              // Nombre de paramètres requis pour ce message
    const MidiParamDef* params = nullptr;// Tableau de params : heap (new[], builder) OU flash (static constexpr)
    size_t paramsCapacity = 0;           // Taille allouée (pour vérification)

    // Cleanup : libère la mémoire allouée (uniquement pour les messages alloués en heap par le builder ;
    // ne PAS appeler sur des messages dont les params pointent vers de la flash).
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
    int8_t altPinType;           // Type de pin alternatif (-1 = aucun, sinon PinType)
    bool implemented;            // true = disponible, false = grisé dans l'UI
    // Note: isComplex supprimé - utiliser additionalPinCount > 0 pour détecter un composant avec pins additionnelles
    bool supportsMidi;           // true = peut envoyer/recevoir MIDI
    bool supportsOsc;            // true = peut envoyer/recevoir OSC
    const char* statusTextTemplate; // Template par défaut pour le texte de statut
    const char* statusValueMappings; // JSON: {"ledMode": {"pwm": "PWM", "onoff": "On/Off"}}
    
    // Champs de formulaire pour l'UI
    uint8_t formFieldCount;     // Nombre de champs de formulaire
    const FormFieldDef* formFields;   // Tableau : heap (new[], builder) OU flash (static constexpr)
    size_t formFieldsCapacity;  // Taille allouée (pour vérification)

    // Pins additionnelles pour composants complexes
    uint8_t additionalPinCount;  // Nombre de pins additionnelles (0 pour simple)
    const AdditionalPinDef* additionalPins; // Tableau : heap (new[], builder) OU flash (static constexpr)
    size_t additionalPinsCapacity;    // Taille allouée (pour vérification)

    // Messages MIDI supportés
    uint8_t midiMessageCount;    // Nombre de types de messages MIDI supportés
    const MidiMessageDef* midiMessages; // Tableau : heap (new[], builder) OU flash (static constexpr)
    size_t midiMessagesCapacity;  // Taille allouée (pour vérification)

    // true  = tableaux alloués en heap par ComponentBuilder → cleanup() doit les delete[].
    // false = tableaux pointant vers de la flash (static constexpr) → cleanup() ne touche à rien.
    bool ownsArrays;

    // Constructeur par défaut
    ComponentDefinition() :
        id(nullptr), displayName(nullptr), icon(nullptr), cardId(nullptr),
        family(ComponentFamily::BASIC), familyName(nullptr),
        type(ComponentType::POTENTIOMETER), pinType(PinType::PIN_DIGITAL), altPinType(-1),
        implemented(false), supportsMidi(false), supportsOsc(false),
        statusTextTemplate(nullptr), statusValueMappings(nullptr),
        formFieldCount(0), formFields(nullptr), formFieldsCapacity(0),
        additionalPinCount(0), additionalPins(nullptr), additionalPinsCapacity(0),
        midiMessageCount(0), midiMessages(nullptr), midiMessagesCapacity(0),
        ownsArrays(true) {}

    // Cleanup : libère la mémoire allouée (récursif).
    // No-op pour les définitions flash (ownsArrays == false) : leurs tableaux sont en .rodata.
    void cleanup() {
        if (!ownsArrays) {
            formFields = nullptr; formFieldsCapacity = 0;
            additionalPins = nullptr; additionalPinsCapacity = 0;
            midiMessages = nullptr; midiMessagesCapacity = 0;
            return;
        }
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
                // const_cast : on possède ces messages (alloués en heap par le builder)
                const_cast<MidiMessageDef&>(midiMessages[i]).cleanup();
            }
            delete[] midiMessages;
            midiMessages = nullptr;
            midiMessagesCapacity = 0;
        }
    }

    /**
     * @brief Écrit une chaîne dans le buffer avec échappement JSON (" \ et caractères de contrôle)
     * @return Nombre d'octets écrits, ou -1 si buffer insuffisant
     */
    static int appendJsonString(char* dest, size_t destSize, const char* src) {
        if (!dest || destSize == 0) return src ? -1 : 0;
        if (!src) { dest[0] = '\0'; return 0; }
        int w = 0;
        for (; *src && (size_t)w < destSize - 1; src++) {
            unsigned char c = (unsigned char)*src;
            if (c == '"') {
                if ((size_t)(w + 2) > destSize) return -1;
                dest[w++] = '\\'; dest[w++] = '"';
            } else if (c == '\\') {
                if ((size_t)(w + 2) > destSize) return -1;
                dest[w++] = '\\'; dest[w++] = '\\';
            } else if (c == '\n') {
                if ((size_t)(w + 2) > destSize) return -1;
                dest[w++] = '\\'; dest[w++] = 'n';
            } else if (c == '\r') {
                if ((size_t)(w + 2) > destSize) return -1;
                dest[w++] = '\\'; dest[w++] = 'r';
            } else if (c == '\t') {
                if ((size_t)(w + 2) > destSize) return -1;
                dest[w++] = '\\'; dest[w++] = 't';
            } else if (c < 0x20 || c == 0x7F) {
                if ((size_t)(w + 6) > destSize) return -1;
                dest[w++] = '\\'; dest[w++] = 'u';
                static const char hex[] = "0123456789abcdef";
                dest[w++] = '0'; dest[w++] = '0';
                dest[w++] = hex[c >> 4];   dest[w++] = hex[c & 0xf];
            } else {
                dest[w++] = (char)c;
            }
        }
        dest[w] = '\0';
        return w;
    }

    /**
     * @brief Écrit dans le buffer via snprintf avec vérification de troncation.
     * snprintf retourne le nombre de caractères VOULUS, pas écrits.
     * Si le buffer est trop petit, written saute au-delà et les écritures suivantes
     * vont dans de la mémoire garbage (cause de corruption JSON).
     * Cette méthode retourne le nombre réellement écrit, ou -1 si tronqué.
     */
    static int safeSnprintf(char* buf, size_t size, const char* fmt, ...)
#if defined(__GNUC__)
        __attribute__((format(printf, 3, 4)))
#endif
    {
        if (size == 0) return -1;
        va_list args;
        va_start(args, fmt);
        int ret = vsnprintf(buf, size, fmt, args);
        va_end(args);
        if (ret < 0 || (size_t)ret >= size) return -1;
        return ret;
    }

    /**
     * @brief Convertit la définition en JSON pour l'API
     * @param buffer Buffer de sortie
     * @param bufferSize Taille du buffer
     * @return Nombre de caractères écrits, 0 si buffer insuffisant
     */
    /**
     * Sérialise en JSON compact avec clés courtes pour réduire la taille des pages.
     * Table de correspondance (compact → complet) utilisée par normalizeDef() dans definitions.js :
     *   dn=displayName  cid=cardId     fn=familyName   pt=pinType    apt=altPinType
     *   impl=implemented  midi=supportsMidi  osc=supportsOsc  apc=additionalPinCount
     *   stt=statusTextTemplate  svm=statusValueMappings  ap=additionalPins
     *   mm=midiMessages  st=statusTemplate(msg)  p=params  ph=placeholder  dv=defaultValue
     *   dmin=defaultMin  dmax=defaultMax  sep=separator  hc=hintClass  dep=dependsOnRole
     *   ff=formFields  ml=maxLength  hp=hintPosition  don=dependsOn  sw=showWhen
     *   wc=wrapperClass  ic=inputClass  w=width  req=required  lb=labelBefore  la=labelAfter
     *   o=options (array inline, sans échappement JSON)
     */
    int toJson(char* buffer, size_t bufferSize) const {
        if (!buffer || bufferSize < 100) return 0;
        int written = 0;
        int r;

        #define W(...) do { r = safeSnprintf(buffer + written, bufferSize - (size_t)written, __VA_ARGS__); if (r < 0) return 0; written += r; } while(0)
        #define WS(str) do { r = appendJsonString(buffer + written, bufferSize - (size_t)written, (str)); if (r < 0) return 0; written += r; } while(0)
        /* WR : écrit une chaîne RAW (déjà du JSON valide, ex: tableau d'options) */
        #define WR(str) do { r = safeSnprintf(buffer + written, bufferSize - (size_t)written, "%s", (str)); if (r < 0) return 0; written += r; } while(0)

        W("{\"id\":\"");   WS(id ? id : "");
        W("\",\"dn\":\""); WS(displayName ? displayName : "");
        W("\",\"cid\":\""); WS(cardId ? cardId : "");
        W("\",\"family\":%d,\"fn\":\"", static_cast<int>(family));
        WS(familyName ? familyName : "");
        W("\",\"pt\":%d", static_cast<int>(pinType));
        if (altPinType >= 0) {
            W(",\"apt\":%d", (int)altPinType);
        }
        W(",\"impl\":%s,\"midi\":%s,\"osc\":%s,\"apc\":%d",
            implemented ? "true" : "false",
            supportsMidi ? "true" : "false",
            supportsOsc ? "true" : "false",
            additionalPinCount);

        if (statusTextTemplate) {
            W(",\"stt\":\"");  WS(statusTextTemplate);  W("\"");
        }

#ifndef NIDMI_COMPONENT_DEFS_LIGHT
        if (statusValueMappings) {
            W(",\"svm\":\"");  WS(statusValueMappings);  W("\"");
        }
#endif

        /* Pins additionnelles */
        if (additionalPinCount > 0 && additionalPins) {
            W(",\"ap\":[");
            for (uint8_t i = 0; i < additionalPinCount && i < additionalPinsCapacity; i++) {
                if (i > 0) W(",");
                W("{\"id\":\"");  WS(additionalPins[i].id ? additionalPins[i].id : "");
                W("\",\"dn\":\"");  WS(additionalPins[i].displayName ? additionalPins[i].displayName : "");
                W("\",\"pt\":%d,\"optional\":%s}",
                    static_cast<int>(additionalPins[i].pinType),
                    additionalPins[i].optional ? "true" : "false");
            }
            W("]");
        }

        /* Messages MIDI */
        if (midiMessageCount > 0 && midiMessages) {
            W(",\"mm\":[");
            for (uint8_t i = 0; i < midiMessageCount && i < midiMessagesCapacity; i++) {
                if (i > 0) W(",");
                W("{\"id\":\"");  WS(midiMessages[i].id ? midiMessages[i].id : "");
                W("\",\"dn\":\"");  WS(midiMessages[i].displayName ? midiMessages[i].displayName : "");
                W("\"");
                if (midiMessages[i].axis) {
                    W(",\"axis\":\"");  WS(midiMessages[i].axis);  W("\"");
                }
                if (midiMessages[i].statusTemplate) {
                    W(",\"st\":\"");  WS(midiMessages[i].statusTemplate);  W("\"");
                }
                if (midiMessages[i].paramCount > 0 && midiMessages[i].params) {
                    W(",\"p\":[");
                    for (uint8_t j = 0; j < midiMessages[i].paramCount && j < midiMessages[i].paramsCapacity; j++) {
                        if (j > 0) W(",");
                        const MidiParamDef& param = midiMessages[i].params[j];
                        W("{\"id\":\"");  WS(param.id ? param.id : "");
                        W("\",\"type\":%d", static_cast<int>(param.type));
                        if (param.label) {
                            W(",\"label\":\"");  WS(param.label);  W("\"");
                        }
                        if (param.type == FieldType::NUMBER || param.type == FieldType::RANGE) {
                            W(",\"min\":%d,\"max\":%d", param.min, param.max);
                        }
                        if (param.placeholder) {
                            W(",\"ph\":\"");  WS(param.placeholder);  W("\"");
                        }
                        if (param.defaultValue) {
                            W(",\"dv\":\"");  WS(param.defaultValue);  W("\"");
                        }
                        if (param.type == FieldType::RANGE) {
                            if (param.defaultMin) { W(",\"dmin\":\"");  WS(param.defaultMin);  W("\""); }
                            if (param.defaultMax) { W(",\"dmax\":\"");  WS(param.defaultMax);  W("\""); }
                            if (param.separator)  { W(",\"sep\":\"");   WS(param.separator);   W("\""); }
                        }
#ifndef NIDMI_COMPONENT_DEFS_LIGHT
                        if (param.type == FieldType::INFO && param.hint) {
                            W(",\"hint\":\"");  WS(param.hint);  W("\"");
                            if (param.hintClass) { W(",\"hc\":\"");  WS(param.hintClass);  W("\""); }
                        }
#endif
                        if (param.dependsOnRole) {
                            W(",\"dep\":\"");  WS(param.dependsOnRole);  W("\"");
                        }
                        if (param.width > 0) {
                            W(",\"w\":%d", param.width);
                        }
                        W("}");
                    }
                    W("]");
                }
                W("}");
            }
            W("]");
        }

        /* Champs de formulaire */
        if (formFieldCount > 0 && formFields) {
            W(",\"ff\":[");
            for (uint8_t i = 0; i < formFieldCount && i < formFieldsCapacity; i++) {
                if (i > 0) W(",");
                const FormFieldDef& field = formFields[i];
                W("{\"id\":\"");  WS(field.id ? field.id : "");
                W("\",\"type\":%d", static_cast<int>(field.type));

                if (field.label)       { W(",\"label\":\"");  WS(field.label);  W("\""); }
                if (field.required)      W(",\"req\":true");
                if (field.placeholder) { W(",\"ph\":\"");  WS(field.placeholder);  W("\""); }
                if (field.maxLength > 0) W(",\"ml\":%d", field.maxLength);
                if (field.pattern)     { W(",\"pattern\":\"");  WS(field.pattern);  W("\""); }

                if (field.type == FieldType::NUMBER || field.type == FieldType::RANGE) {
                    W(",\"min\":%d,\"max\":%d", field.min, field.max);
                    if (field.step != 1) W(",\"step\":%d", field.step);
                }
                /* options écrit inline (raw JSON array) : évite le double échappement des " */
                if (field.type == FieldType::SELECT && field.options) {
                    W(",\"o\":");  WR(field.options);
                }
                if (field.type == FieldType::RANGE && field.separator) {
                    W(",\"sep\":\"");  WS(field.separator);  W("\"");
                }
                if (field.defaultValue) { W(",\"dv\":\"");  WS(field.defaultValue);  W("\""); }

#ifndef NIDMI_COMPONENT_DEFS_LIGHT
                if (field.hintPosition != HintPosition::NONE && field.hint) {
                    W(",\"hp\":%d,\"hint\":\"", static_cast<int>(field.hintPosition));
                    WS(field.hint);  W("\"");
                    if (field.hintClass) { W(",\"hc\":\"");  WS(field.hintClass);  W("\""); }
                }
#endif
                if (field.dependsOn) {
                    W(",\"don\":\"");  WS(field.dependsOn);  W("\"");
                    if (field.showWhen) { W(",\"sw\":\"");  WS(field.showWhen);  W("\""); }
                }
                if (field.wrapperClass) { W(",\"wc\":\"");  WS(field.wrapperClass);  W("\""); }
                if (field.inputClass)   { W(",\"ic\":\"");  WS(field.inputClass);    W("\""); }
                if (field.width > 0)      W(",\"w\":%d", field.width);
                if (field.labelBefore)  { W(",\"lb\":\"");  WS(field.labelBefore);  W("\""); }
                if (field.labelAfter)   { W(",\"la\":\"");  WS(field.labelAfter);   W("\""); }

                W("}");
            }
            W("]");
        }

        W("}");

        #undef W
        #undef WS
        #undef WR
        return written;
    }
};
