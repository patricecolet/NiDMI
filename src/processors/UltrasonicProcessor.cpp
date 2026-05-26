#include "UltrasonicProcessor.h"
#include "ProcessorRegistry.h"
#include "../midi/handlers/MidiOutputCoordinator.h"
#include "../utils/AxisUtils.h"

// Lecture distance en mm sur un capteur ultrasonique utilisant une seule pin
static uint16_t readDistanceMM(uint8_t pin, uint16_t lastValid) {
    // Séquence standard: impulsion de 10 µs, puis pulseIn
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    delayMicroseconds(2);
    digitalWrite(pin, HIGH);
    delayMicroseconds(10);
    digitalWrite(pin, LOW);

    pinMode(pin, INPUT);
    long duration = pulseIn(pin, HIGH, 30000); // timeout 30ms

    if (duration == 0) {
        // Pas de nouvelle lecture valide → garder la dernière
        return lastValid;
    }

    // Conversion temps → distance (mm)
    double halfDuration = static_cast<double>(duration) / 2.0;
    uint16_t distance = static_cast<uint16_t>(halfDuration * 0.343); // ≈ mm

    // Filtrer valeurs extrêmes (0–4000 mm)
    if (distance > 0 && distance < 4000) {
        return distance;
    }
    return lastValid;
}

void UltrasonicProcessor::process(
    const ComponentConfig &config,
    ComponentState &state,
    AnalogFilter &filter,
    MidiSender *midi_sender,
    OSCQueue &osc_queue
) {
    // GPIO valide ?
    if (config.gpio >= 255 || config.gpio > 48) {
        return;
    }

    // Lecture distance brute (mm)
    uint16_t raw_distance = readDistanceMM(config.gpio, state.last_value);

    // Mettre à jour alpha du filtre (défaut 5, pas de config spécifique encore)
    uint8_t intensity = 5; // Défaut
    filter.setAlphaFromIntensity(intensity);

    // FILTRAGE d'abord (comme le potentiomètre)
    uint16_t filtered_raw_value;
    if (config.msg_type == MidiMessageType::NOTE_SWEEP) {
        filtered_raw_value = filter.processMedianAndLowpass(raw_distance);
    } else {
        filtered_raw_value = filter.process(raw_distance);
    }

    // Mapping min/max en mm (on réutilise customInt1/customInt2 pour stocker les distances)
    uint16_t mapped_value;
    uint16_t dist_min = config.customInt1;
    uint16_t dist_max = config.customInt2;

    // Si non configuré (0,0) → 0–4000 mm
    if (dist_min == 0 && dist_max == 0) {
        dist_min = 0;
        dist_max = 4000;
    }

    if (dist_max > dist_min) {
        if (filtered_raw_value < dist_min) {
            mapped_value = 0;
        } else if (filtered_raw_value > dist_max) {
            mapped_value = 4095;
        } else {
            mapped_value = static_cast<uint16_t>(
                map(filtered_raw_value, dist_min, dist_max, 0, 4095)
            );
        }
    } else {
        // Seuils invalides → pas de mapping, on normalise sur la plage 0–4000 mm
        mapped_value = static_cast<uint16_t>(
            map(filtered_raw_value, 0, 4000, 0, 4095)
        );
    }

    uint16_t filtered_value = mapped_value;

    // ===== Traitement NOTE_SWEEP (copie de PotentiometerProcessor) =====
    if (config.msg_type == MidiMessageType::NOTE_SWEEP) {
        const uint16_t sweepOffMs = effectiveNoteSweepAutoOffMs(config.rtpNoteSweepAutoOffDelay);
        // Auto-off : coupe la note si le délai est écoulé, mais conserve state.last_note
        // pour éviter un retrigger si la position pointe toujours sur la même note.
        if (sweepOffMs > 0 &&
            state.last_note != 255 &&
            state.note_on_time > 0) {
            uint32_t elapsed = millis() - state.note_on_time;
            if (elapsed >= sweepOffMs) {
                midi_sender->sendNoteOff(config.midi_channel, state.last_note, 0);
                state.note_on_time = 0;
            }
        }

        if (!state.hysteresis.update(filtered_value)) {
            return;
        }

        uint8_t stable_midi_value = state.hysteresis.getValue();

        uint8_t noteMin = config.rtpNoteMin;
        uint8_t noteMax = config.rtpNoteMax;
        uint8_t newNote;

        if (stable_midi_value == 0) {
            newNote = 255;
        } else {
            newNote = map(stable_midi_value, 1, 127, noteMin, noteMax);
        }

        if (newNote == state.last_note) {
            return;
        }

        // Note différente : couper l'ancienne uniquement si encore active.
        if (state.last_note != 255 && state.note_on_time > 0) {
            midi_sender->sendNoteOff(config.midi_channel, state.last_note, 0);
        }

        if (newNote != 255) {
            midi_sender->sendNoteOn(config.midi_channel, newNote, config.rtpNoteVelFix);
            state.note_on_time = millis();
        } else {
            state.note_on_time = 0;
        }

        state.last_note = newNote;
        state.last_value = stable_midi_value;
        state.last_time = millis();
        state.last_raw_value_u32 = filtered_value;
        state.last_midi_value_u8 = stable_midi_value;
        state.last_telemetry_ts = state.last_time;

        MidiOutputCoordinator::sendOsc(osc_queue, config, stable_midi_value, filtered_value);
        return;
    }

    // ===== Traitement standard (CC, PB, Aftertouch…) =====
    if (!state.hysteresis.update(filtered_value)) {
        return;
    }

    uint8_t midi_value = state.hysteresis.getValue();  // 0–127

    // Plage MIDI configurable
    if (config.midiCcRangeMin != 0 || config.midiCcRangeMax != 127) {
        midi_value = map(midi_value, 0, 127, config.midiCcRangeMin, config.midiCcRangeMax);
    }

    if (midi_value == state.last_value) {
        return;
    }

    uint16_t raw_value_for_handler =
        (config.msg_type == MidiMessageType::PITCH_BEND)
            ? filtered_raw_value  // Pitch Bend : on garde la valeur "brute" filtrée
            : filtered_value;     // Autres : normalisée 0–4095

    MidiOutputCoordinator::sendMidiAndOsc(
        midi_sender, osc_queue, config, midi_value, raw_value_for_handler
    );

    state.last_value = midi_value;
    state.last_time = millis();
    state.last_raw_value_u32 = raw_value_for_handler;
    state.last_midi_value_u8 = midi_value;
    state.last_telemetry_ts = state.last_time;
}

// Wrapper pour ProcessorRegistry
static void ultrasonicProcessWrapper(
    const ComponentConfig &config,
    ComponentState &state,
    AnalogFilter *filter,
    MidiSender *midi_sender,
    OSCQueue &osc_queue
) {
    if (filter == nullptr) {
        return; // Ultrasonique nécessite un filtre (comme le potentiomètre)
    }
    UltrasonicProcessor::process(config, state, *filter, midi_sender, osc_queue);
}

// Enregistrement automatique
static bool ultrasonicRegistered = ProcessorRegistry::registerProcessor(
    ComponentType::ULTRASONIC,
    ultrasonicProcessWrapper
);

