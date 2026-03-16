#pragma once

#include <Arduino.h>
#include "../components/ComponentTypes.h"  // Définitions communes
#include "../midi/MidiSender.h"
#include "../midi/MidiMessageType.h"
#include "../osc/OSCQueue.h"

/**
 * @brief Processeur pour les boutons
 * 
 * Gère la lecture digitale, l'anti-rebond, la détection de transitions
 * et l'envoi MIDI/OSC selon les modes (press_release, toggle, pulse).
 */
class ButtonProcessor {
public:
    /**
     * @brief Traiter un bouton
     * @param config Configuration du composant
     * @param state État runtime du composant
     * @param midi_sender Envoyeur MIDI
     * @param osc_queue Queue OSC pour l'envoi batch
     */
    static void process(
        const ComponentConfig& config,
        ComponentState& state,
        MidiSender* midi_sender,
        OSCQueue& osc_queue
    );
};
