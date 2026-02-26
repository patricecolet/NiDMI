#pragma once

#include "../components/ComponentTypes.h"
#include "../midi/MidiSender.h"
#include "../osc/OSCQueue.h"

/**
 * @brief Processeur pour le MPR121 (touch capacitif 12 canaux I2C)
 *
 * Lit le statut des 12 électrodes, détecte les fronts touch/release
 * et envoie Note On/Off ou CC selon la configuration.
 */
class Mpr121Processor {
public:
    /**
     * @brief Traite un composant MPR121
     */
    static void process(
        const ComponentConfig& config,
        ComponentState& state,
        MidiSender* midi_sender,
        OSCQueue& osc_queue
    );
};
