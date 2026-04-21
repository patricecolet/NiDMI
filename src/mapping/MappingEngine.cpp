#include "MappingEngine.h"
#include "../Globals.h"
#include "../midi/MidiSender.h"
#include "../server/ServerCore.h"

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

void FluxRegistry::debug() {
    Serial.printf("[FluxRegistry] Registry contents (%d entries):\n", count);
    for (int i = 0; i < count; i++) {
        Serial.printf("  [%d] '%s' = %.2f\n", i, entries[i].name, entries[i].value);
    }
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
    
    // Remove comments (everything after //)
    int commentIdx = s.indexOf("//");
    if (commentIdx != -1) {
        s = s.substring(0, commentIdx);
    }
    
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
                    Serial.printf("[MappingEngine] r(\"%s\") found → value = %.2f\n", target.c_str(), current);
                } else {
                    // Fallback: keep current input value if named source does not exist.
                    Serial.printf("[MappingEngine] WARNING: r(\"%s\") not found! Available:\n", target.c_str());
                    FluxRegistry::debug();  // Print what's in the registry
                    Serial.printf("[MappingEngine] Fallback to input value: %.2f\n", current);
                }
            }
        }
        // f("...") — Load a constant numeric value (variable declaration)
        else if (seg.startsWith("f(\"")) {
            int closeIdx = seg.indexOf("\")");
            if (closeIdx != -1) {
                String valueStr = seg.substring(3, closeIdx);
                current = valueStr.toFloat();
                Serial.printf("[MappingEngine] Loaded constant value: %.2f\n", current);
            }
        }
        // s("...") — Store current flux value as a variable (send/store to registry)
        else if (seg.startsWith("s(\"")) {
            int closeIdx = seg.indexOf("\")");
            if (closeIdx != -1) {
                String varName = seg.substring(3, closeIdx);
                FluxRegistry::update(varName.c_str(), current);
                Serial.printf("[MappingEngine] s(\"%s\"): stored value %.2f\n", varName.c_str(), current);
                Serial.printf("[MappingEngine] Registry now has %d entries\n", FluxRegistry::count);
            }
        } 
        else if (seg.startsWith("*(")) { 
            int closeIdx = seg.indexOf(")");
            if (closeIdx != -1) {
                String operand = seg.substring(2, closeIdx);
                float m;
                // Check if operand is a variable reference r("name")
                if (operand.startsWith("r(\"")) {
                    int varCloseIdx = operand.indexOf("\")");
                    if (varCloseIdx != -1) {
                        String varName = operand.substring(3, varCloseIdx);
                        m = FluxRegistry::get(varName.c_str());
                        Serial.printf("[MappingEngine] Multiply by variable '%s' = %.2f\n", varName.c_str(), m);
                    } else {
                        m = 0;
                    }
                } else {
                    m = operand.toFloat();
                }
                current *= m;
            }
        }
        else if (seg.startsWith("+(")) { 
            int closeIdx = seg.indexOf(")");
            if (closeIdx != -1) {
                String operand = seg.substring(2, closeIdx);
                float a;
                // Check if operand is a variable reference r("name")
                if (operand.startsWith("r(\"")) {
                    int varCloseIdx = operand.indexOf("\")");
                    if (varCloseIdx != -1) {
                        String varName = operand.substring(3, varCloseIdx);
                        a = FluxRegistry::get(varName.c_str());
                        Serial.printf("[MappingEngine] Add variable '%s' = %.2f\n", varName.c_str(), a);
                    } else {
                        a = 0;
                    }
                } else {
                    a = operand.toFloat();
                }
                current += a;
            }
        }
        else if (seg.startsWith("-(")) { 
            int closeIdx = seg.indexOf(")");
            if (closeIdx != -1) {
                String operand = seg.substring(2, closeIdx);
                float s;
                // Check if operand is a variable reference r("name")
                if (operand.startsWith("r(\"")) {
                    int varCloseIdx = operand.indexOf("\")");
                    if (varCloseIdx != -1) {
                        String varName = operand.substring(3, varCloseIdx);
                        s = FluxRegistry::get(varName.c_str());
                        Serial.printf("[MappingEngine] Subtract variable '%s' = %.2f\n", varName.c_str(), s);
                    } else {
                        s = 0;
                    }
                } else {
                    s = operand.toFloat();
                }
                current -= s;
            }
        }
        else if (seg.startsWith("/(")) { 
            int closeIdx = seg.indexOf(")");
            if (closeIdx != -1) {
                String operand = seg.substring(2, closeIdx);
                float d;
                // Check if operand is a variable reference r("name")
                if (operand.startsWith("r(\"")) {
                    int varCloseIdx = operand.indexOf("\")");
                    if (varCloseIdx != -1) {
                        String varName = operand.substring(3, varCloseIdx);
                        d = FluxRegistry::get(varName.c_str());
                        Serial.printf("[MappingEngine] Divide by variable '%s' = %.2f\n", varName.c_str(), d);
                    } else {
                        d = 1; // Avoid division by zero
                    }
                } else {
                    d = operand.toFloat();
                }
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
        else if (seg.startsWith("seq.out(")) {
            int closeIdx = seg.indexOf(')');
            if (closeIdx != -1) {
                String source = "seq";
                int firstQuote = seg.indexOf('"');
                if (firstQuote != -1 && firstQuote < closeIdx) {
                    int secondQuote = seg.indexOf('"', firstQuote + 1);
                    if (secondQuote != -1 && secondQuote < closeIdx) {
                        source = seg.substring(firstQuote + 1, secondQuote);
                    }
                }
                String json = "{\"type\":\"seq_event\",\"source\":\"" + source + "\",\"value\":" + String(current > 0 ? 1 : 0) + "}";
                serverCore.websocket().textAll(json);
                Serial.printf("[MappingEngine] Sent seq event source='%s' value=%d\n", source.c_str(), current > 0 ? 1 : 0);
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
        // Pass-through functions that don't modify the flux but can be chained
        else if (seg.startsWith("print(")) {
            int closeIdx = seg.indexOf(')');
            if (closeIdx != -1) {
                Serial.printf("[MappingEngine] DEBUG: flux value = %.2f\n", current);
            }
        }
        else if (seg.startsWith("graph(")) {
            int closeIdx = seg.indexOf(')');
            if (closeIdx != -1) {
                // Graph function - currently just logs the value
                Serial.printf("[MappingEngine] GRAPH: %.2f\n", current);
            }
        }
        // s() without parameters - pass-through storage reference
        else if (seg == "s()") {
            // Pass-through, does nothing
            Serial.printf("[MappingEngine] Pass-through s() - current value: %.2f\n", current);
        }
        // ctl.out() variations - these are already handled above but ensure pass-through
        // osc.out() - pass-through function (handled separately)
        else if (seg.startsWith("osc.out(")) {
            int firstQuote = seg.indexOf('"');
            int lastQuote = seg.lastIndexOf('"');
            int closeIdx = seg.indexOf(')');
            if (firstQuote != -1 && lastQuote != -1 && closeIdx != -1 && firstQuote < lastQuote) {
                String oscPath = seg.substring(firstQuote + 1, lastQuote);
                // OSC output - flux passes through
                Serial.printf("[MappingEngine] OSC out: path='%s' value=%.2f\n", oscPath.c_str(), current);
                // TODO: Implement actual OSC sending if needed
            }
        }

        if (end == -1) break;
        start = end + 1;
        end = s.indexOf(':', start);
    }
}