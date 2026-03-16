#pragma once

#include <Arduino.h>
#include "../components/ComponentTypes.h"  // Définitions communes
#include "../midi/MidiSender.h"
#include "../midi/MidiMessageType.h"
#include "../osc/OSCQueue.h"
#include "../utils/PinMapper.h"
#include "../utils/AnalogFilter.h"

/**
 * @brief Processeur pour les potentiomètres
 * 
 * Gère la lecture, le filtrage, l'hystérésis et l'envoi MIDI/OSC
 * pour les potentiomètres (GPIO normales, non-MUX).
 */
class PotentiometerProcessor {
public:
    /**
     * @brief Traiter un potentiomètre
     * @param config Configuration du composant
     * @param state État runtime du composant
     * @param filter Filtre analogique
     * @param midi_sender Envoyeur MIDI
     * @param osc_queue Queue OSC pour l'envoi batch
     */
    static void process(
        const ComponentConfig& config,
        ComponentState& state,
        AnalogFilter& filter,
        MidiSender* midi_sender,
        OSCQueue& osc_queue
    );
};
