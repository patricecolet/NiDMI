#include "VelostatProcessor.h"
#include "ProcessorRegistry.h"
#include "../components/ComponentTypes.h"
#include "../components/basic/VelostatDef.h"
#include "../midi/handlers/MidiOutputCoordinator.h"
#include "../utils/PinMapper.h"

void VelostatProcessor::process(
    const ComponentConfig& config,
    ComponentState& state,
    AnalogFilter& filter,
    MidiSender* midi_sender,
    OSCQueue& osc_queue
) {
    // Vérifier que le GPIO est valide et a un ADC
    if (config.gpio >= 255 || config.gpio > 48) {
        return;
    }
    
    if (!PinMapper::hasAdc(config.gpio)) {
        return;
    }
    
    // Lecture analogique (0-4095 pour ESP32)
    uint16_t sensor_value = analogRead(config.gpio);
    
    // Mettre à jour alpha du filtre selon filter_intensity (1-10)
    uint8_t intensity = 5; // Valeur par défaut
    uint16_t velocity_threshold = 50;
    uint8_t aftertouch_threshold = 4;
    
    if (config.specificConfig.velostat) {
        intensity = config.specificConfig.velostat->filter_intensity;
        if (intensity == 0) intensity = 5; // Valeur par défaut si non configuré
        velocity_threshold = config.specificConfig.velostat->velocityThreshold;
        aftertouch_threshold = config.specificConfig.velostat->aftertouchThreshold;
    }
    
    filter.setAlphaFromIntensity(intensity);
    
    // Filtrer la valeur brute
    uint16_t filtered_value = filter.process(sensor_value);
    
    // Mapper la valeur filtrée vers la vélocité (1-127)
    // Mapping depuis [velocityThreshold, 4095] vers [1, 127]
    uint8_t velocity = 0;
    if (filtered_value > velocity_threshold) {
        if (filtered_value >= 4095) {
            velocity = 127;
        } else {
            velocity = map(filtered_value, velocity_threshold, 4095, 1, 127);
            if (velocity < 1) velocity = 1;
            if (velocity > 127) velocity = 127;
        }
    }
    
    uint8_t note = config.midi_param;
    uint8_t channel = config.midi_channel;
    
    // Gestion Note On/Off
    bool note_is_on = (state.last_note != 255);
    
    if (filtered_value > velocity_threshold && !note_is_on) {
        // Note On
        if (midi_sender) {
            midi_sender->sendNoteOn(channel, note, velocity);
        }
        state.last_note = note;
        state.last_value = velocity;
        state.last_raw_value_u32 = filtered_value;
        state.last_midi_value_u8 = velocity;
        state.last_telemetry_ts = millis();
        state.last_aftertouch = velocity; // Initialiser last_aftertouch avec la vélocité
        state.last_time = millis();
        
        // Envoyer aussi en OSC si configuré
        MidiOutputCoordinator::sendOsc(osc_queue, config, velocity, filtered_value);
    } else if (filtered_value <= velocity_threshold && note_is_on) {
        // Note Off
        if (midi_sender) {
            midi_sender->sendNoteOff(channel, note, 0);
        }
        state.last_note = 255;
        state.last_value = 0;
        state.last_raw_value_u32 = filtered_value;
        state.last_midi_value_u8 = 0;
        state.last_telemetry_ts = millis();
        state.last_aftertouch = 0;
        
        // Envoyer aussi en OSC si configuré
        MidiOutputCoordinator::sendOsc(osc_queue, config, 0, filtered_value);
    } else if (note_is_on && velocity > 0) {
        // Note est active : envoyer Key Pressure (Polyphonic Aftertouch)
        // Filtrer la vélocité pour stabilité (utiliser le filtre analogique comme MovingAverage)
        // On utilise directement la vélocité mappée
        
        // Envoyer seulement si changement significatif
        int velocity_diff = (velocity > state.last_aftertouch) ? 
                           (velocity - state.last_aftertouch) : 
                           (state.last_aftertouch - velocity);
        if (velocity_diff > aftertouch_threshold) {
            if (midi_sender) {
                midi_sender->sendKeyPressure(channel, note, velocity);
            }
            state.last_aftertouch = velocity;
            state.last_raw_value_u32 = filtered_value;
            state.last_midi_value_u8 = velocity;
            state.last_telemetry_ts = millis();
            state.last_time = millis();
            
            // Envoyer aussi en OSC si configuré
            MidiOutputCoordinator::sendOsc(osc_queue, config, velocity, filtered_value);
        }
    }
}

// Wrapper pour normaliser la signature
static void processWrapper(
    const ComponentConfig& config,
    ComponentState& state,
    AnalogFilter* filter,
    MidiSender* midi_sender,
    OSCQueue& osc_queue
) {
    if (filter == nullptr) {
        return;
    }
    VelostatProcessor::process(config, state, *filter, midi_sender, osc_queue);
}

// Enregistrement automatique au chargement du module
static bool registered = ProcessorRegistry::registerProcessor(
    ComponentType::VELOSTAT,
    processWrapper
);
