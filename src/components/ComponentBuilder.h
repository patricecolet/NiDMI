#pragma once

#include "ComponentDefinition.h"
#include <cstring>
#include <utility>  // pour std::move

/**
 * @file ComponentBuilder.h
 * @brief Builder pour simplifier la création de ComponentDefinition
 * 
 * Permet de créer des ComponentDefinition avec moins de code répétitif.
 * Note: Ce builder alloue de la mémoire dynamiquement, donc les ComponentDefinition
 * créées doivent être nettoyées avec cleanup().
 */

namespace Components {

/**
 * @brief Builder pour ComponentDefinition
 * 
 * Utilisation:
 * @code
 * ComponentDefinition def = ComponentBuilder()
 *     .setBasicInfo("led", "LED", "cardLed")
 *     .setFamily(ComponentFamily::BASIC, "Basic")
 *     .setType(ComponentType::LED, PinType::PIN_DIGITAL)
 *     .setCapabilities(true, false)
 *     .build();
 * @endcode
 */
class ComponentBuilder {
private:
    ComponentDefinition def;
    
public:
    ComponentBuilder() {
        // Initialiser avec des valeurs par défaut
        def = ComponentDefinition();
    }
    
    /**
     * @brief Définit les informations de base (id, displayName, cardId)
     */
    ComponentBuilder& setBasicInfo(const char* id, const char* displayName, const char* cardId) {
        def.id = id;
        def.displayName = displayName;
        def.cardId = cardId;
        return *this;
    }
    
    /**
     * @brief Définit la famille et le nom de famille
     */
    ComponentBuilder& setFamily(ComponentFamily family, const char* familyName) {
        def.family = family;
        def.familyName = familyName;
        return *this;
    }
    
    /**
     * @brief Définit le type et le type de pin
     */
    ComponentBuilder& setType(ComponentType type, PinType pinType) {
        def.type = type;
        def.pinType = pinType;
        return *this;
    }

    ComponentBuilder& setAltPinType(PinType alt) {
        def.altPinType = static_cast<int8_t>(alt);
        return *this;
    }
    
    /**
     * @brief Définit les capacités (MIDI, OSC)
     */
    ComponentBuilder& setCapabilities(bool supportsMidi, bool supportsOsc) {
        def.supportsMidi = supportsMidi;
        def.supportsOsc = supportsOsc;
        return *this;
    }
    
    /**
     * @brief Définit si le composant est implémenté
     */
    ComponentBuilder& setImplemented(bool implemented) {
        def.implemented = implemented;
        return *this;
    }
    
    /**
     * @brief Définit le template de statut
     */
    ComponentBuilder& setStatusTemplate(const char* template_) {
        def.statusTextTemplate = template_;
        return *this;
    }
    
    /**
     * @brief Définit les mappings de valeurs de statut (JSON)
     */
    ComponentBuilder& setStatusValueMappings(const char* mappings) {
        def.statusValueMappings = mappings;
        return *this;
    }
    
    /**
     * @brief Ajoute un champ de formulaire
     * Note: Les champs doivent être ajoutés avant build()
     */
    ComponentBuilder& addFormField(const FormFieldDef& field) {
        // Pour simplifier, on alloue un nouveau tableau à chaque ajout
        // (pas optimal mais simple pour l'instant)
        uint8_t newCount = def.formFieldCount + 1;
        FormFieldDef* newFields = new FormFieldDef[newCount];
        
        // Copier les anciens champs
        for (uint8_t i = 0; i < def.formFieldCount; i++) {
            newFields[i] = def.formFields[i];
        }
        
        // Ajouter le nouveau champ
        newFields[def.formFieldCount] = field;
        
        // Libérer l'ancien tableau
        if (def.formFields) {
            delete[] def.formFields;
        }
        
        // Mettre à jour
        def.formFields = newFields;
        def.formFieldCount = newCount;
        def.formFieldsCapacity = newCount;
        
        return *this;
    }
    
    /**
     * @brief Ajoute un message MIDI
     * Note: Les messages doivent être ajoutés avant build()
     */
    ComponentBuilder& addMidiMessage(const MidiMessageDef& msg) {
        // Même logique que pour les formFields
        uint8_t newCount = def.midiMessageCount + 1;
        MidiMessageDef* newMessages = new MidiMessageDef[newCount];
        
        // Copier les anciens messages avec copie profonde des params
        for (uint8_t i = 0; i < def.midiMessageCount; i++) {
            newMessages[i].id = def.midiMessages[i].id;
            newMessages[i].displayName = def.midiMessages[i].displayName;
            newMessages[i].statusTemplate = def.midiMessages[i].statusTemplate;
            newMessages[i].axis = def.midiMessages[i].axis;
            newMessages[i].paramCount = def.midiMessages[i].paramCount;
            newMessages[i].paramsCapacity = def.midiMessages[i].paramCount;
            // Copie profonde des params
            if (def.midiMessages[i].paramCount > 0 && def.midiMessages[i].params) {
                newMessages[i].params = new MidiParamDef[def.midiMessages[i].paramCount];
                for (uint8_t j = 0; j < def.midiMessages[i].paramCount; j++) {
                    newMessages[i].params[j] = def.midiMessages[i].params[j];
                }
            } else {
                newMessages[i].params = nullptr;
            }
        }
        
        // Ajouter le nouveau message avec copie profonde des params
        newMessages[def.midiMessageCount].id = msg.id;
        newMessages[def.midiMessageCount].displayName = msg.displayName;
        newMessages[def.midiMessageCount].statusTemplate = msg.statusTemplate;
        newMessages[def.midiMessageCount].axis = msg.axis;
        newMessages[def.midiMessageCount].paramCount = msg.paramCount;
        newMessages[def.midiMessageCount].paramsCapacity = msg.paramCount;
        if (msg.paramCount > 0 && msg.params) {
            newMessages[def.midiMessageCount].params = new MidiParamDef[msg.paramCount];
            for (uint8_t j = 0; j < msg.paramCount; j++) {
                newMessages[def.midiMessageCount].params[j] = msg.params[j];
            }
        } else {
            newMessages[def.midiMessageCount].params = nullptr;
        }
        
        // Libérer l'ancien tableau
        if (def.midiMessages) {
            // Libérer les params de chaque message
            for (uint8_t i = 0; i < def.midiMessagesCapacity; i++) {
                def.midiMessages[i].cleanup();
            }
            delete[] def.midiMessages;
        }
        
        // Mettre à jour
        def.midiMessages = newMessages;
        def.midiMessageCount = newCount;
        def.midiMessagesCapacity = newCount;
        
        return *this;
    }
    
    /**
     * @brief Définit les pins additionnelles
     */
    ComponentBuilder& setAdditionalPins(const AdditionalPinDef* pins, uint8_t count) {
        if (def.additionalPins) {
            delete[] def.additionalPins;
        }
        
        def.additionalPinCount = count;
        def.additionalPinsCapacity = count;
        if (count > 0) {
            def.additionalPins = new AdditionalPinDef[count];
            for (uint8_t i = 0; i < count; i++) {
                def.additionalPins[i] = pins[i];
            }
        } else {
            def.additionalPins = nullptr;
        }
        
        return *this;
    }
    
    /**
     * @brief Construit la ComponentDefinition finale avec copie profonde
     * 
     * Note: Cette méthode fait une copie profonde de tous les tableaux alloués
     * pour que la ComponentDefinition retournée soit indépendante du builder.
     * Le builder conserve sa mémoire (qui sera libérée à sa destruction normale).
     */
    ComponentDefinition build() {
        // S'assurer que icon est nullptr (par défaut)
        def.icon = nullptr;
        
        // S'assurer que les champs additionnels sont initialisés si non définis
        if (def.formFieldCount == 0) {
            def.formFields = nullptr;
            def.formFieldsCapacity = 0;
        }
        if (def.midiMessageCount == 0) {
            def.midiMessages = nullptr;
            def.midiMessagesCapacity = 0;
        }
        if (def.additionalPinCount == 0) {
            def.additionalPins = nullptr;
            def.additionalPinsCapacity = 0;
        }
        
        // Faire une copie profonde pour que la ComponentDefinition soit indépendante
        ComponentDefinition result;
        result.id = def.id;
        result.displayName = def.displayName;
        result.icon = def.icon;
        result.cardId = def.cardId;
        result.family = def.family;
        result.familyName = def.familyName;
        result.type = def.type;
        result.pinType = def.pinType;
        result.implemented = def.implemented;
        result.supportsMidi = def.supportsMidi;
        result.supportsOsc = def.supportsOsc;
        result.statusTextTemplate = def.statusTextTemplate;
        result.statusValueMappings = def.statusValueMappings;
        
        // Copier les formFields
        result.formFieldCount = def.formFieldCount;
        result.formFieldsCapacity = def.formFieldCount;
        if (def.formFieldCount > 0 && def.formFields) {
            result.formFields = new FormFieldDef[def.formFieldCount];
            for (uint8_t i = 0; i < def.formFieldCount; i++) {
                result.formFields[i] = def.formFields[i];
            }
        } else {
            result.formFields = nullptr;
        }
        
        // Copier les additionalPins
        result.additionalPinCount = def.additionalPinCount;
        result.additionalPinsCapacity = def.additionalPinCount;
        if (def.additionalPinCount > 0 && def.additionalPins) {
            result.additionalPins = new AdditionalPinDef[def.additionalPinCount];
            for (uint8_t i = 0; i < def.additionalPinCount; i++) {
                result.additionalPins[i] = def.additionalPins[i];
            }
        } else {
            result.additionalPins = nullptr;
        }
        
        // Copier les midiMessages (copie profonde avec params)
        result.midiMessageCount = def.midiMessageCount;
        result.midiMessagesCapacity = def.midiMessageCount;
        if (def.midiMessageCount > 0 && def.midiMessages) {
            result.midiMessages = new MidiMessageDef[def.midiMessageCount];
            for (uint8_t i = 0; i < def.midiMessageCount; i++) {
                result.midiMessages[i].id = def.midiMessages[i].id;
                result.midiMessages[i].displayName = def.midiMessages[i].displayName;
                result.midiMessages[i].statusTemplate = def.midiMessages[i].statusTemplate;
                result.midiMessages[i].axis = def.midiMessages[i].axis;
                result.midiMessages[i].paramCount = def.midiMessages[i].paramCount;
                result.midiMessages[i].paramsCapacity = def.midiMessages[i].paramCount;
                if (def.midiMessages[i].paramCount > 0 && def.midiMessages[i].params) {
                    result.midiMessages[i].params = new MidiParamDef[def.midiMessages[i].paramCount];
                    for (uint8_t j = 0; j < def.midiMessages[i].paramCount; j++) {
                        result.midiMessages[i].params[j] = def.midiMessages[i].params[j];
                    }
                } else {
                    result.midiMessages[i].params = nullptr;
                }
            }
        } else {
            result.midiMessages = nullptr;
        }
        
        // Réinitialiser le builder pour éviter que ses pointeurs pointent vers la mémoire copiée
        // (même si le builder sera détruit immédiatement après, c'est une bonne pratique)
        def.formFields = nullptr;
        def.formFieldCount = 0;
        def.formFieldsCapacity = 0;
        def.midiMessages = nullptr;
        def.midiMessageCount = 0;
        def.midiMessagesCapacity = 0;
        def.additionalPins = nullptr;
        def.additionalPinCount = 0;
        def.additionalPinsCapacity = 0;
        
        return result;
    }
};

} // namespace Components
