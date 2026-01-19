#include "PotentiometerProcessor.h"
#include "ProcessorRegistry.h"
#include "../components/ComponentTypes.h"  // Définitions communes
#include "../midi/handlers/MidiOutputCoordinator.h"

void PotentiometerProcessor::process(
    const ComponentConfig& config,
    ComponentState& state,
    AnalogFilter& filter,
    MidiSender* midi_sender,
    OSCQueue& osc_queue
) {
    // ===== TRAITEMENT POUR GPIO NORMALES (non-MUX) =====
    // GPIO normal : vérifier qu'il est valide et a un ADC
    if (config.gpio >= 255 || config.gpio > 48) {
        return;
    }
    
    if (!PinMapper::hasAdc(config.gpio)) {
        return;
    }
    
    // Lecture analogique directe
    uint16_t raw_value = analogRead(config.gpio);
    
    // Appliquer le mapping min/max (comme dans le MUX)
    uint16_t mapped_value;
    uint16_t analog_min = config.potMin;
    uint16_t analog_max = config.potMax;
    
    // Si les seuils ne sont pas configurés (0,0), utiliser les valeurs par défaut
    if (analog_min == 0 && analog_max == 0) {
        analog_min = 0;
        analog_max = 4095;
    }
    
    if (analog_max > analog_min) {
        int32_t raw = raw_value;
        int32_t min_val = analog_min;
        int32_t max_val = analog_max;
        
        if (raw < min_val) raw = min_val;
        if (raw > max_val) raw = max_val;
        
        mapped_value = (uint16_t)map(raw, min_val, max_val, 0, 4095);
    } else {
        mapped_value = raw_value; // Pas de mapping si seuils invalides
    }
    
    // Mettre à jour alpha du filtre selon filter_intensity (1-10)
    uint8_t intensity = config.filter_intensity;
    if (intensity == 0) intensity = 5; // Valeur par défaut si non configuré
    filter.setAlphaFromIntensity(intensity);
    
    // Filtrage : médian + passe-bas agressif pour NOTE_SWEEP, sinon filtre normal
    uint16_t filtered_value;
    if (config.msg_type == MidiMessageType::NOTE_SWEEP) {
        filtered_value = filter.processMedianAndLowpass(mapped_value);
    } else {
        filtered_value = filter.process(mapped_value);
    }
    
    // ===== TRAITEMENT SPÉCIAL NOTE_SWEEP (GPIO normales) =====
    if (config.msg_type == MidiMessageType::NOTE_SWEEP) {
        // 1. Vérifier l'auto-off AVANT tout
        if (config.rtpNoteSweepAutoOffDelay > 0 && 
            state.last_note != 255 && 
            state.note_on_time > 0) {
            uint32_t elapsed = millis() - state.note_on_time;
            if (elapsed >= config.rtpNoteSweepAutoOffDelay) {
                midi_sender->sendNoteOff(config.midi_channel, state.last_note, 0);
                state.last_note = 255;
                state.note_on_time = 0;
            }
        }
        
        // 2. Appliquer l'hystérésis directement sur filtered_value (0-4095)
        // L'hystérésis réduit automatiquement vers 0-127
        if (!state.hysteresis.update(filtered_value)) {
            return; // Valeur stable, rien à faire
        }
        
        // 3. Utiliser la valeur stabilisée par l'hystérésis (déjà 0-127)
        uint8_t stable_midi_value = state.hysteresis.getValue();
        
        // 4. Calculer la nouvelle note
        uint8_t noteMin = config.rtpNoteMin;
        uint8_t noteMax = config.rtpNoteMax;
        uint8_t newNote;
        
        if (stable_midi_value == 0) {
            newNote = 255; // Pas de note (potentiomètre à zéro)
        } else {
            newNote = map(stable_midi_value, 1, 127, noteMin, noteMax);
        }
        
        // 5. Si la note est identique à la précédente, ne rien faire
        if (newNote == state.last_note) {
            return;
        }
        
        // 6. Éteindre l'ancienne note si elle existe
        if (state.last_note != 255) {
            midi_sender->sendNoteOff(config.midi_channel, state.last_note, 0);
        }
        
        // 7. Jouer la nouvelle note (sauf si 255)
        if (newNote != 255) {
            midi_sender->sendNoteOn(config.midi_channel, newNote, config.rtpNoteVelFix);
            state.note_on_time = (config.rtpNoteSweepAutoOffDelay > 0) ? millis() : 0;
        } else {
            state.note_on_time = 0;
        }
        
        // 8. Mettre à jour l'état
        state.last_note = newNote;
        state.last_value = stable_midi_value;
        state.last_time = millis();
        
        // 9. OSC si activé (même valeur que MIDI)
        MidiOutputCoordinator::sendOsc(osc_queue, config, stable_midi_value, filtered_value);
        
        return; // Traitement NOTE_SWEEP GPIO normale terminé
    }
    
    // ===== TRAITEMENT STANDARD pour GPIO normales (autres types) =====
    // Appliquer l'hystérésis directement sur filtered_value (0-4095)
    // L'hystérésis réduit automatiquement vers 0-127 (méthode Control-Surface)
    if (!state.hysteresis.update(filtered_value)) {
        return; // Valeur stable, rien à faire
    }
    
    uint8_t midi_value = state.hysteresis.getValue();  // Déjà 0-127
    
    // Comparer avec la dernière valeur ENVOYÉE (comme le MUX)
    if (midi_value == state.last_value) {
        return; // Pas de changement, ne pas envoyer
    }
    
    // Envoyer MIDI et OSC via le coordinateur (évite les redondances)
    MidiOutputCoordinator::sendMidiAndOsc(midi_sender, osc_queue, config, midi_value, filtered_value);
    
    // Mettre à jour last_value UNE SEULE FOIS après l'envoi (comme le MUX)
    state.last_value = midi_value;
    state.last_time = millis();
}

// Wrapper pour normaliser la signature (filter par pointeur au lieu de référence)
static void processWrapper(
    const ComponentConfig& config,
    ComponentState& state,
    AnalogFilter* filter,
    MidiSender* midi_sender,
    OSCQueue& osc_queue
) {
    if (filter == nullptr) {
        return; // Potentiomètre nécessite un filtre
    }
    PotentiometerProcessor::process(config, state, *filter, midi_sender, osc_queue);
}

// Enregistrement automatique au chargement du module
static bool registered = ProcessorRegistry::registerProcessor(
    ComponentType::POTENTIOMETER,
    processWrapper
);
