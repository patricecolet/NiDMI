#include "../../src/midi/MidiMessageType.h"

MidiMessageType stringToMidiMessageType(const String&) { return MidiMessageType::NOTE; }
String midiMessageTypeToString(MidiMessageType) { return String("note"); }
