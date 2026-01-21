#pragma once

#include <Arduino.h>
#include "../components/ComponentTypes.h"
#include "../utils/AnalogFilter.h"
#include "../midi/MidiSender.h"
#include "../osc/OSCQueue.h"

/**
 * @brief Processeur pour capteur tactile ESP32-S3
 *
 * Lit la valeur tactile (0-4095, plus bas = touché) sur un pin touch ESP32-S3,
 * applique filtrage + mapping min/max, puis envoie MIDI/OSC
 * (CC, Pitch Bend, Aftertouch, Note + Key Pressure, Note simple).
 * 
 * Compatible uniquement avec ESP32-S3, pas ESP32-C3.
 */
class TouchProcessor {
public:
    static void process(
        const ComponentConfig &config,
        ComponentState &state,
        AnalogFilter &filter,
        MidiSender *midi_sender,
        OSCQueue &osc_queue
    );
};
