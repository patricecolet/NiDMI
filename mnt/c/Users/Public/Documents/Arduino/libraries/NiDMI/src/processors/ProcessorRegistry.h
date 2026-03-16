#pragma once

#include <Arduino.h>
#include "../components/ComponentTypes.h"  // Définitions communes (ComponentType, ComponentConfig, ComponentState)
#include "../utils/AnalogFilter.h"

// Forward declarations
class MidiSender;
class OSCQueue;

/**
 * @brief Registre de processeurs de composants
 * 
 * Permet d'enregistrer et d'appeler dynamiquement les processeurs
 * pour chaque type de composant. Extensible pour supporter des centaines
 * de types de composants sans modifier ComponentManager.
 * 
 * Usage pour ajouter un nouveau composant :
 * ```cpp
 * // Dans MyComponentProcessor.cpp
 * #include "ProcessorRegistry.h"
 * 
 * // Wrapper pour normaliser la signature
 * static void processWrapper(
 *     const ComponentConfig& config,
 *     ComponentState& state,
 *     AnalogFilter* filter,
 *     MidiSender* midi_sender,
 *     OSCQueue& osc_queue
 * ) {
 *     MyComponentProcessor::process(config, state, midi_sender, osc_queue);
 * }
 * 
 * // Enregistrement automatique au chargement
 * static bool registered = ProcessorRegistry::registerProcessor(
 *     ComponentType::MY_COMPONENT,
 *     processWrapper
 * );
 * ```
 */
class ProcessorRegistry {
public:
    /**
     * @brief Type de fonction pour traiter un composant
     * Signature normalisée : (config, state, filter*, midi_sender, osc_queue)
     * Le filter peut être nullptr pour les composants non-analogiques
     */
    typedef void (*ProcessorFunc)(
        const ComponentConfig& config,
        ComponentState& state,
        AnalogFilter* filter,  // nullptr pour les composants non-analogiques
        MidiSender* midi_sender,
        OSCQueue& osc_queue
    );
    
    /**
     * @brief Enregistrer un processeur pour un type de composant
     * @param type Type de composant
     * @param processor Fonction de traitement
     * @return true si l'enregistrement a réussi
     */
    static bool registerProcessor(ComponentType type, ProcessorFunc processor);
    
    /**
     * @brief Traiter un composant en utilisant le processeur enregistré
     * @param type Type de composant
     * @param config Configuration du composant
     * @param state État du composant
     * @param filter Filtre analogique (peut être nullptr pour les composants non-analogiques)
     * @param midi_sender Envoyeur MIDI
     * @param osc_queue Queue OSC
     * @return true si le processeur existe et a été appelé, false sinon
     */
    static bool process(
        ComponentType type,
        const ComponentConfig& config,
        ComponentState& state,
        AnalogFilter* filter,
        MidiSender* midi_sender,
        OSCQueue& osc_queue
    );
    
    /**
     * @brief Vérifier si un processeur est enregistré pour un type
     * @param type Type de composant
     * @return true si un processeur est enregistré
     */
    static bool hasProcessor(ComponentType type);
};
