#include <Arduino.h>
#include <cstdint>
#include <cstddef>
#include "../storage/SequencerFileStore.h"

#define MAX_STEPS 32
#define MAX_NOTES 4

struct Note {
    uint8_t pitch;
    uint8_t velocity;
};

struct Step {
    uint8_t noteCount;
    Note notes[MAX_NOTES];
    uint8_t measure;
};

Step steps[MAX_STEPS];
uint8_t stepCount = 0;

void parseNidmid(uint8_t* data, size_t len) {
    stepCount = 0;

    uint8_t currentMeasure = 0;
    size_t i = 0;

    while (i < len && stepCount < MAX_STEPS) {
        uint8_t b = data[i++];

        if (b == 0xFF) {
            currentMeasure++;
            continue;
        }

        uint8_t noteCount = b;
        if (noteCount > MAX_NOTES) noteCount = MAX_NOTES;

        Step& s = steps[stepCount];
        s.noteCount = noteCount;
        s.measure = currentMeasure;

        for (int j = 0; j < noteCount && i + 1 < len; j++) {
            s.notes[j].pitch = data[i++];
            s.notes[j].velocity = data[i++];
        }

        stepCount++;
    }

    Serial.printf("Loaded %d steps\n", stepCount);
}

bool reloadSequencerFromStorage() {
    auto& store = SequencerFileStore::getInstance();
    SequencerReadResult result = store.read();
    
    if (result.status != SequencerStoreResult::SUCCESS) {
        Serial.printf("[SequencerProcessor] Error: Could not read sequence from storage (status:%d)\n", (int)result.status);
        return false;
    }
    
    if (!result.checksumValid) {
        Serial.println("[SequencerProcessor] Error: Sequence checksum invalid");
        return false;
    }
    
    parseNidmid(result.data.data(), result.data.size());
    return true;
}