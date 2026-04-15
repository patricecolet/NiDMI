#include "MappingEngine.h"
#include "../midi/MidiSender.h"

// INITIALISATION DES STATICS (Obligatoire dans le .cpp)
FluxRegistry::Entry FluxRegistry::entries[32];
int FluxRegistry::count = 0;

void FluxRegistry::update(const char* name, float val) {
    if (!name || name[0] == '\0') return;
    for (int i = 0; i < count; i++) {
        if (strcmp(entries[i].name, name) == 0) {
            entries[i].value = val;
            return;
        }
    }
    if (count < 32) {
        strlcpy(entries[count].name, name, 16);
        entries[count].value = val;
        count++;
    }
}

float FluxRegistry::get(const char* name) {
    for (int i = 0; i < count; i++) {
        if (strcmp(entries[i].name, name) == 0) return entries[i].value;
    }
    return 0.0f;
}

bool FluxRegistry::has(const char* name) {
    if (!name || name[0] == '\0') return false;
    for (int i = 0; i < count; i++) {
        if (strcmp(entries[i].name, name) == 0) return true;
    }
    return false;
}
// Helper pour envoyer CC via MIDI
static void sendMidiControlChange(uint8_t cc, uint8_t value, uint8_t chan, MidiSender* sender) {
    if (sender) {
        sender->sendControlChange(chan, cc, value);
        Serial.printf("[MappingEngine] Sent MIDI CC:%d Chan:%d Val:%d\n", cc, chan, value);
    } else {
        Serial.printf("[MappingEngine] WARNING: No MIDI sender available for CC\n");
    }
}

// Helper pour envoyer Note On via MIDI
static void sendMidiNoteOn(uint8_t note, uint8_t channel, uint8_t velocity, MidiSender* sender) {
    if (sender) {
        sender->sendNoteOn(channel, note, velocity);
        Serial.printf("[MappingEngine] Sent MIDI Note On: Note:%d Chan:%d Vel:%d\n", note, channel, velocity);
    } else {
        Serial.printf("[MappingEngine] WARNING: No MIDI sender available for Note On\n");
    }
}

// Helper pour envoyer Note Off via MIDI
static void sendMidiNoteOff(uint8_t note, uint8_t channel, uint8_t velocity, MidiSender* sender) {
    if (sender) {
        sender->sendNoteOff(channel, note, velocity);
        Serial.printf("[MappingEngine] Sent MIDI Note Off: Note:%d Chan:%d Vel:%d\n", note, channel, velocity);
    } else {
        Serial.printf("[MappingEngine] WARNING: No MIDI sender available for Note Off\n");
    }
}
void MappingEngine::execute(const char* script, float inputVal, MidiSender* midi_sender) {
    if (!script || script[0] == '\0') return;

    float current = inputVal;
    String s = String(script);
    int start = 0;
    int end = s.indexOf(':');

    Serial.printf("[MappingEngine] execute script='%s' input=%.2f\n", script, inputVal);
    while (start < (int)s.length()) {
        int actualEnd = (end == -1) ? s.length() : end;
        String seg = s.substring(start, actualEnd);
        seg.trim();

        if (seg.startsWith("r(\"")) { 
            int closeIdx = seg.indexOf("\")");
            if (closeIdx != -1) {
                String target = seg.substring(3, closeIdx);
                if (FluxRegistry::has(target.c_str())) {
                    current = FluxRegistry::get(target.c_str());
                } else {
                    // Fallback: keep current input value if named source does not exist.
                    Serial.printf("[MappingEngine] WARNING: source '%s' not found, fallback to input %.2f\n", target.c_str(), current);
                }
            }
        } 
        else if (seg.startsWith("*(")) { 
            int closeIdx = seg.indexOf(")");
            if (closeIdx != -1) {
                float m = seg.substring(2, closeIdx).toFloat();
                current *= m;
            }
        }
        else if (seg.startsWith("+(")) { 
            int closeIdx = seg.indexOf(")");
            if (closeIdx != -1) {
                float a = seg.substring(2, closeIdx).toFloat();
                current += a;
            }
        }
        else if (seg.startsWith("-(")) { 
            int closeIdx = seg.indexOf(")");
            if (closeIdx != -1) {
                float s = seg.substring(2, closeIdx).toFloat();
                current -= s;
            }
        }
        else if (seg.startsWith("/(")) { 
            int closeIdx = seg.indexOf(")");
            if (closeIdx != -1) {
                float d = seg.substring(2, closeIdx).toFloat();
                if (d != 0) current /= d;
            }
        }
        else if (seg.startsWith("ctl.out(")) { 
            int comma = seg.indexOf(',');
            int closeIdx = seg.indexOf(')');
            if (comma != -1 && closeIdx != -1) {
                int cc = seg.substring(8, comma).toInt();
                int chan = seg.substring(comma + 1, closeIdx).toInt();
                
                // Clamp value to MIDI range (0-127)
                uint8_t midiValue = (uint8_t)constrain(current, 0, 127);
                
                // Send via MIDI helper
                sendMidiControlChange((uint8_t)cc, midiValue, (uint8_t)chan, midi_sender);
            }
        }
        else if (seg.startsWith("note.on(")) {
            int comma1 = seg.indexOf(',');
            int closeIdx = seg.indexOf(')');
            if (comma1 != -1 && closeIdx != -1) {
                int note = seg.substring(8, comma1).toInt();
                int chan = seg.substring(comma1 + 1, closeIdx).toInt();
                note = constrain(note, 0, 127);
                chan = constrain(chan, 1, 16);
                uint8_t vel = (uint8_t)constrain((int)current, 0, 127);
                sendMidiNoteOn((uint8_t)note, (uint8_t)chan, vel, midi_sender);
            }
        }
        // note.off(note, chan) — sends note off with velocity 127
        else if (seg.startsWith("note.off(")) {
            int comma1 = seg.indexOf(',');
            int closeIdx = seg.indexOf(')');
            if (comma1 != -1 && closeIdx != -1) {
                int note = seg.substring(9, comma1).toInt();
                int chan = seg.substring(comma1 + 1, closeIdx).toInt();
                note = constrain(note, 0, 127);
                chan = constrain(chan, 1, 16);
                uint8_t vel = 127;  // Fixed velocity for note off
                sendMidiNoteOff((uint8_t)note, (uint8_t)chan, vel, midi_sender);
            }
        }
        // note.out(note, chan) — sends note.on when pressed (>0), note.off with vel 0 when released (<=0)
        else if (seg.startsWith("note.out(")) {
            int comma1 = seg.indexOf(',');
            int closeIdx = seg.indexOf(')');
            if (comma1 != -1 && closeIdx != -1) {
                int note = seg.substring(9, comma1).toInt();
                int chan = seg.substring(comma1 + 1, closeIdx).toInt();
                note = constrain(note, 0, 127);
                chan = constrain(chan, 1, 16);
                if (current > 0) {
                    uint8_t vel = (uint8_t)constrain((int)current, 1, 127);  // At least 1 for note on
                    sendMidiNoteOn((uint8_t)note, (uint8_t)chan, vel, midi_sender);
                } else {
                    sendMidiNoteOff((uint8_t)note, (uint8_t)chan, 0, midi_sender);
                }
            }
        }

        if (end == -1) break;
        start = end + 1;
        end = s.indexOf(':', start);
    }
}