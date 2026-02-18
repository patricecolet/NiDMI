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
    
    // Gérer les displayNames avec axe : "Control Change (Axe X)" -> CONTROL_CHANGE
    if (str.startsWith("Control Change")) return MidiMessageType::CONTROL_CHANGE;
    if (str.startsWith("Pitch Bend")) return MidiMessageType::PITCH_BEND;
    if (str.startsWith("Aftertouch")) return MidiMessageType::AFTERTOUCH;
    if (str.startsWith("Note (balayage)") || str.startsWith("Note Sweep")) return MidiMessageType::NOTE_SWEEP;
    if (str.startsWith("Note +") || str.startsWith("Note+")) return MidiMessageType::NOTE_VELOCITY;
    if (str.startsWith("Note")) return MidiMessageType::NOTE;
    
    // Gérer les IDs bruts (cc, pitchBend, aftertouch, noteSweep)
    if (str == "cc") return MidiMessageType::CONTROL_CHANGE;
    if (str == "pitchBend") return MidiMessageType::PITCH_BEND;
    if (str == "aftertouch") return MidiMessageType::AFTERTOUCH;
    if (str == "noteSweep") return MidiMessageType::NOTE_SWEEP;
    if (str == "noteVelocity") return MidiMessageType::NOTE_VELOCITY;
    if (str == "programChange") return MidiMessageType::PROGRAM_CHANGE;
    
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

