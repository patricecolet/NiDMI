#include "PotentiometerProcessor.h"
#include "ProcessorRegistry.h"
#include "../components/ComponentTypes.h"  // Définitions communes

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
    
    // Mettre à jour alpha du filtre selon filter_intensity (1-10)
    uint8_t intensity = config.filter_intensity;
    if (intensity == 0) intensity = 5; // Valeur par défaut si non configuré
    filter.setAlphaFromIntensity(intensity);
    
    // Filtrage : médian + passe-bas agressif pour NOTE_SWEEP, sinon filtre normal
    uint16_t filtered_value;
    if (config.msg_type == MidiMessageType::NOTE_SWEEP) {
        filtered_value = filter.processMedianAndLowpass(raw_value);
    } else {
        filtered_value = filter.process(raw_value);
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
        if (config.flags & 0x02) {
            String oscAddress = (config.osc_address[0] != '\0') ? String(config.osc_address) : "/note";
            if (config.flags & 0x04) {
                osc_queue.enqueueMidi(oscAddress, stable_midi_value, config.midi_param, config.midi_channel);
            } else {
                osc_queue.enqueueFloat(oscAddress, stable_midi_value / 127.0f);
            }
        }
        
        return; // Traitement NOTE_SWEEP GPIO normale terminé
    }
    
    // ===== TRAITEMENT STANDARD pour GPIO normales (autres types) =====
    // Appliquer l'hystérésis directement sur filtered_value (0-4095)
    // L'hystérésis réduit automatiquement vers 0-127 (méthode Control-Surface)
    if (!state.hysteresis.update(filtered_value)) {
        return; // Valeur stable, rien à faire
    }
    
    uint8_t midi_value = state.hysteresis.getValue();  // Déjà 0-127
    
    // Envoyer seulement si changement significatif (seuil de 1 pour valeurs 0-127)
    if (abs((int)midi_value - (int)state.last_value) >= 1) {
        state.last_value = midi_value;  // Mettre à jour avant d'envoyer
        // Envoyer le message MIDI selon le type configuré
        switch (config.msg_type) {
            case MidiMessageType::CONTROL_CHANGE:
                midi_sender->sendControlChange(config.midi_channel, config.midi_param, midi_value);
                break;
            case MidiMessageType::PITCH_BEND: {
                // Pitch Bend: 0-127 → -8192 à +8191 (signé, centre=0)
                int pitchBend = map(midi_value, 0, 127, -8192, 8191);
                midi_sender->sendPitchBend(config.midi_channel, pitchBend);
                break;
            }
            case MidiMessageType::AFTERTOUCH:
                midi_sender->sendAftertouch(config.midi_channel, midi_value);
                break;
            case MidiMessageType::NOTE_VELOCITY:
                // Note + vélocité: envoyer Note On avec vélocité variable
                if (midi_value > 0) {
                    midi_sender->sendNoteOn(config.midi_channel, config.midi_param, midi_value);
                } else {
                    midi_sender->sendNoteOff(config.midi_channel, config.midi_param, 0);
                }
                break;
            case MidiMessageType::PROGRAM_CHANGE:
                midi_sender->sendProgramChange(config.midi_channel, midi_value);
                break;
            default:
                // Par défaut: Control Change
                midi_sender->sendControlChange(config.midi_channel, config.midi_param, midi_value);
                break;
        }
        
        // Envoyer OSC si activé (via queue prioritaire)
        if (config.flags & 0x02) { // Bit OSC enabled
            // Utiliser l'adresse OSC configurée (ou défaut si vide)
            String oscAddress = (config.osc_address[0] != '\0') ? String(config.osc_address) : "/ctl";
            
            if (config.flags & 0x04) { // Format MIDI
                osc_queue.enqueueMidi(oscAddress, midi_value, config.midi_param, config.midi_channel);
            } else { // Format float
                osc_queue.enqueueFloat(oscAddress, midi_value / 127.0f);
            }
        }
        
        // Mettre à jour last_value
        state.last_value = midi_value;
        state.last_time = millis();
    }
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
