#include "MappingEngine.h"

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

void MappingEngine::execute(const char* script, float inputVal) {
    if (!script || script[0] == '\0') return;

    float current = inputVal;
    String s = String(script);
    int start = 0;
    int end = s.indexOf(':');

    while (start < (int)s.length()) {
        int actualEnd = (end == -1) ? s.length() : end;
        String seg = s.substring(start, actualEnd);
        seg.trim();

        if (seg.startsWith("r(\"")) { 
            int closeIdx = seg.indexOf("\")");
            if (closeIdx != -1) {
                String target = seg.substring(3, closeIdx);
                current = FluxRegistry::get(target.c_str());
            }
        } 
        else if (seg.startsWith("*(")) { 
            int closeIdx = seg.indexOf(")");
            if (closeIdx != -1) {
                float m = seg.substring(2, closeIdx).toFloat();
                current *= m;
            }
        }
        else if (seg.startsWith("ctl.out(")) { 
            int comma = seg.indexOf(',');
            int closeIdx = seg.indexOf(')');
            if (comma != -1 && closeIdx != -1) {
                int cc = seg.substring(8, comma).toInt();
                int chan = seg.substring(comma + 1, closeIdx).toInt();
                
                // Debug log
                Serial.printf("[Mapping] MIDI CC:%d Chan:%d Val:%d\n", cc, chan, (uint8_t)current);
                
                // Note: Call your MIDI instance here when ready
            }
        }

        if (end == -1) break;
        start = end + 1;
        end = s.indexOf(':', start);
    }
}