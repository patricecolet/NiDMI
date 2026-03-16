// Types de messages MIDI supportés
#pragma once

#include <Arduino.h>

enum class MidiMessageType : uint8_t {
    NOTE = 0,                    // Note On/Off
    CONTROL_CHANGE = 1,          // Control Change (CC)
    PROGRAM_CHANGE = 2,          // Program Change
    PITCH_BEND = 3,              // Pitch Bend (0-16383, centre=8192)
    AFTERTOUCH = 4,              // Aftertouch (Channel Pressure)
    NOTE_VELOCITY = 5,            // Note + vélocité (Note avec vélocité variable)
    NOTE_SWEEP = 6,               // Note (balayage) - Note avec balayage de notes
    CLOCK = 7,                   // MIDI Clock (messages système)
    TAP_TEMPO = 8                // Tap Tempo (MIDI Clock avec timing)
};

// Fonction utilitaire pour convertir string vers enum
MidiMessageType stringToMidiMessageType(const String& str);
String midiMessageTypeToString(MidiMessageType type);

