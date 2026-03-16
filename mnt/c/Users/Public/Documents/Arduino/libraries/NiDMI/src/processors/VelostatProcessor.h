#pragma once

#include "../components/ComponentTypes.h"
#include "../midi/MidiSender.h"
#include "../osc/OSCQueue.h"
#include "../utils/AnalogFilter.h"

/**
 * @brief Processeur pour les capteurs Velostat
 * 
 * Gère la lecture analogique, le filtrage, et l'envoi de
 * Note On/Off avec Key Pressure (Polyphonic Aftertouch).
 */
class VelostatProcessor {
public:
    /**
     * @brief Traite un composant Velostat
     * 
     * @param config Configuration du composant
     * @param state État du composant (note active, dernière valeur, etc.)
     * @param filter Filtre analogique pour lisser les valeurs
     * @param midi_sender Sender MIDI pour envoyer les messages
     * @param osc_queue Queue OSC pour envoyer les messages OSC
     */
    static void process(
        const ComponentConfig& config,
        ComponentState& state,
        AnalogFilter& filter,
        MidiSender* midi_sender,
        OSCQueue& osc_queue
    );
};
