// Templates de messages MIDI compacts
#pragma once

#include <Arduino.h>

enum class MidiMsgType : uint8_t { Note, CC, PC };

template<MidiMsgType T>
struct MidiMessage {
    union {
        struct { uint8_t note, velocity, channel; } note;
        struct { uint8_t cc, value, channel; } cc;
        struct { uint8_t program, channel; } pc;
    } data;
};


