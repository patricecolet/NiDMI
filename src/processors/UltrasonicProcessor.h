#pragma once

#include <Arduino.h>
#include "../components/ComponentTypes.h"
#include "../utils/AnalogFilter.h"
#include "../midi/MidiSender.h"
#include "../osc/OSCQueue.h"

/**
 * @brief Processeur pour capteur ultrasonique
 *
 * Lit la distance en mm sur un capteur ultrasonique,
 * applique filtrage + mapping min/max, puis envoie MIDI/OSC
 * comme un potentiomètre (CC, Pitch Bend, Aftertouch, Note Sweep).
 */
class UltrasonicProcessor {
public:
    static void process(
        const ComponentConfig &config,
        ComponentState &state,
        AnalogFilter &filter,
        MidiSender *midi_sender,
        OSCQueue &osc_queue
    );
};

