#pragma once

#include <Arduino.h>
#include "../../components/ComponentDefinition.h"

/**
 * @file ComplexHandler.h
 * @brief Interface générique pour gérer les composants avec additionalPins
 * 
 * Ce système permet de gérer dynamiquement n'importe quel composant
 * avec additionalPins sans hardcoding dans PinAPI.cpp.
 * 
 * Chaque composant avec additionalPins implémente cette interface :
 * - MuxHandler pour les multiplexeurs (hc4067, hc4051)
 * - MatrixHandler pour les matrices de boutons (futur)
 * - etc.
 * 
 * Architecture inspirée de ProcessorRegistry pour les composants simples.
 */

/**
 * @brief Structure pour passer les données d'un composant complexe
 */
struct ComplexComponentData {
    const ComponentDefinition* def;          // Définition du composant
    const char* pinLabel;                     // Label de la pin principale (ex: "A0")
    uint8_t mainPinGpio;                      // GPIO de la pin principale
    
    // Pins additionnelles : map id -> GPIO (255 = non connecté)
    struct AdditionalPinValue {
        const char* id;                       // ID de la pin (ex: "s0", "en")
        uint8_t gpio;                         // GPIO (255 = non connecté)
    };
    AdditionalPinValue* additionalPins;       // Tableau alloué dynamiquement
    uint8_t additionalPinCount;               // Nombre de pins additionnelles
    
    // FormFields : map id -> valeur (string)
    struct FormFieldValue {
        const char* id;                       // ID du champ (ex: "muxMin", "muxMax")
        String value;                         // Valeur (string)
    };
    FormFieldValue* formFields;               // Tableau alloué dynamiquement
    uint8_t formFieldCount;                   // Nombre de formFields
    
    // Paramètres MIDI : map id -> valeur (string)
    struct MidiParamValue {
        const char* id;                       // ID du paramètre (ex: "rtpCc", "rtpChan")
        String value;                         // Valeur (string)
    };
    MidiParamValue* midiParams;               // Tableau alloué dynamiquement
    uint8_t midiParamCount;                   // Nombre de paramètres MIDI
    
    // Paramètres OSC/Debug
    bool oscEnabled;
    String oscAddress;
    String oscFormat;
    bool dbgEnabled;
    String dbgHeader;
};

/**
 * @brief Interface abstraite pour gérer un type de composant complexe
 */
class ComplexHandler {
public:
    virtual ~ComplexHandler() = default;
    
    /**
     * @brief Retourne l'ID du composant géré (ex: "hc4067", "hc4051")
     */
    virtual const char* getComponentId() const = 0;
    
    /**
     * @brief Ajoute ou met à jour un composant
     * @param data Données du composant (additionalPins, formFields, MIDI params)
     * @return true si succès, false sinon
     */
    virtual bool addComponent(const ComplexComponentData& data) = 0;
    
    /**
     * @brief Supprime un composant
     * @param pinLabel Label de la pin principale (ex: "A0")
     * @param mainPinGpio GPIO de la pin principale
     * @return true si succès, false sinon
     */
    virtual bool removeComponent(const char* pinLabel, uint8_t mainPinGpio) = 0;
    
    /**
     * @brief Retourne les informations d'un composant existant
     * @param pinLabel Label de la pin principale
     * @param mainPinGpio GPIO de la pin principale
     * @param json Buffer de sortie pour le JSON (déjà commencé avec pinLabel et role)
     * @return true si trouvé, false sinon
     */
    virtual bool getComponentInfo(const char* pinLabel, uint8_t mainPinGpio, String& json) = 0;
    
    /**
     * @brief Vérifie si un GPIO est utilisé par ce type de composant
     * @param gpio GPIO à vérifier
     * @return true si utilisé, false sinon
     */
    virtual bool isGpioUsed(uint8_t gpio) const = 0;
    
    /**
     * @brief Retourne le nombre de composants de ce type configurés
     */
    virtual uint8_t getComponentCount() const = 0;
};
