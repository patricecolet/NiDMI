#include "SequencerAPI.h"

// External data
extern int stepCount;

struct Note {
    int pitch;
    int velocity;
};

struct Step {
    int measure;
    int noteCount;
    Note notes[16];
};

extern Step steps[];

extern void parseNidmid(uint8_t *data, size_t len);

void setupSequencerAPI(AsyncWebServer& server) {

    server.on("/api/sequencer/load",
        HTTP_POST,
        [](AsyncWebServerRequest *request){},
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t, size_t){

            Serial.printf("📥 Received %d bytes\n", len);

            parseNidmid(data, len);

            request->send(200, "application/json", "{\"status\":\"ok\"}");
        }
    );

    server.on("/api/sequencer/view", HTTP_GET, [](AsyncWebServerRequest *request){

        String json = "{ \"steps\":[";

        for (int i = 0; i < stepCount; i++) {
            if (i > 0) json += ",";

            json += "{";
            json += "\"measure\":" + String(steps[i].measure) + ",";
            json += "\"notes\":[";

            for (int j = 0; j < steps[i].noteCount; j++) {
                if (j > 0) json += ",";
                json += "{";
                json += "\"pitch\":" + String(steps[i].notes[j].pitch) + ",";
                json += "\"velocity\":" + String(steps[i].notes[j].velocity);
                json += "}";
            }

            json += "]";
            json += "}";
        }

        json += "]}";

        request->send(200, "application/json", json);
    });
}