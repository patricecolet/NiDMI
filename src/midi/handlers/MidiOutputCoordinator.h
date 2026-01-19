#pragma once

#include <Arduino.h>
#include "../../components/ComponentTypes.h"
#include "../MidiSender.h"
#include "../../osc/OSCQueue.h"
#include "MidiMessageHandler.h"
#include "OscMessageHandler.h"

/**
 * @file MidiOutputCoordinator.h
 * @brief Coordinateur pour l'envoi MIDI et OSC unifié
 * 
 * Cette classe coordonne les handlers MIDI et OSC pour éviter les redondances
 * dans les processeurs. Elle gère automatiquement :
 * - L'envoi MIDI selon le type de message
 * - L'envoi OSC selon le format (RAW/MIDI/Float)
 * - Les conversions de valeurs (pitch bend, etc.)
 */

class MidiOutputCoordinator {
public:
    /**
     * @brief Envoie un message MIDI selon le type configuré
     * 
     * @param midi_sender Interface d'envoi MIDI
     * @param config Configuration du composant
     * @param value Valeur MIDI (0-127)
     */
    static void sendMidi(
        MidiSender* midi_sender,
        const ComponentConfig& config,
        uint8_t value
    );
    
    /**
     * @brief Envoie un message OSC selon le format configuré
     * 
     * @param osc_queue Queue OSC
     * @param config Configuration du composant
     * @param midi_value Valeur MIDI (0-127)
     * @param raw_value Valeur brute (0-4095) pour format RAW
     */
    static void sendOsc(
        OSCQueue& osc_queue,
        const ComponentConfig& config,
        uint8_t midi_value,
        uint16_t raw_value = 0
    );
    
    /**
     * @brief Envoie MIDI + OSC en une seule fois
     * 
     * @param midi_sender Interface d'envoi MIDI
     * @param osc_queue Queue OSC
     * @param config Configuration du composant
     * @param midi_value Valeur MIDI (0-127)
     * @param raw_value Valeur brute (0-4095) pour format RAW OSC
     */
    static void sendMidiAndOsc(
        MidiSender* midi_sender,
        OSCQueue& osc_queue,
        const ComponentConfig& config,
        uint8_t midi_value,
        uint16_t raw_value = 0
    );
};
