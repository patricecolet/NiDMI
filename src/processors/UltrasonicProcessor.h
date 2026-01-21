#pragma once

#include <Arduino.h>
#include "../components/ComponentTypes.h"
#include "../utils/AnalogFilter.h"
#include "../midi/MidiSender.h"
#include "../osc/OSCQueue.h"

/**
 * @brief Processeur pour capteur ultrasonique HC-SR04+
 *
 * Lit la distance en mm sur un capteur ultrasonique HC-SR04+ (3.3V),
 * applique filtrage + mapping min/max, puis envoie MIDI/OSC
 * comme un potentiomètre (CC, Pitch Bend, Aftertouch, Note Sweep).
 * 
 * Compatible uniquement avec HC-SR04+ (3.3V), pas le HC-SR04 standard (5V).
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

