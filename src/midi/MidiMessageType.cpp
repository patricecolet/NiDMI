#include "MidiMessageType.h"

MidiMessageType stringToMidiMessageType(const String& str) {
    if (str == "Note") return MidiMessageType::NOTE;
    if (str == "Control Change") return MidiMessageType::CONTROL_CHANGE;
    if (str == "Program Change") return MidiMessageType::PROGRAM_CHANGE;
    if (str == "Pitch Bend") return MidiMessageType::PITCH_BEND;
    if (str == "Aftertouch (Channel)") return MidiMessageType::AFTERTOUCH;
    if (str == "Note + vélocité") return MidiMessageType::NOTE_VELOCITY;
    if (str == "Note (balayage)") return MidiMessageType::NOTE_SWEEP;
    if (str == "Clock") return MidiMessageType::CLOCK;
    if (str == "Tap Tempo") return MidiMessageType::TAP_TEMPO;
    
    // Défaut selon le contexte (sera géré par le code appelant)
    return MidiMessageType::NOTE;
}

String midiMessageTypeToString(MidiMessageType type) {
    switch (type) {
        case MidiMessageType::NOTE: return "Note";
        case MidiMessageType::CONTROL_CHANGE: return "Control Change";
        case MidiMessageType::PROGRAM_CHANGE: return "Program Change";
        case MidiMessageType::PITCH_BEND: return "Pitch Bend";
        case MidiMessageType::AFTERTOUCH: return "Aftertouch (Channel)";
        case MidiMessageType::NOTE_VELOCITY: return "Note + vélocité";
        case MidiMessageType::NOTE_SWEEP: return "Note (balayage)";
        case MidiMessageType::CLOCK: return "Clock";
        case MidiMessageType::TAP_TEMPO: return "Tap Tempo";
        default: return "Note";
    }
}

