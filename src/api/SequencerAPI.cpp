#include "SequencerAPI.h"
#include "storage/SequencerFileStore.h"
#include <LittleFS.h>

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
extern bool reloadSequencerFromStorage();

void setupSequencerAPI(AsyncWebServer& server) {

    // ========== Upload endpoint (POST) ==========
    server.on("/api/sequencer/upload", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            Serial.printf("[SequencerAPI] 📥 POST /upload onRequest (contentLength=%u)\n", request->contentLength());
        },
        nullptr,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            Serial.printf("[SequencerAPI] 📥 Body callback: index=%zu, len=%zu, total=%zu\n", index, len, total);
            
            auto& store = SequencerFileStore::getInstance();
            
            // Initialiser l'upload au PREMIER appel du callback, quel que soit index
            // (parfois index ne commence pas à 0)
            if (!store.isUploadInProgress()) {
                Serial.printf("[SequencerAPI] 🟢 Initializing upload (index=%zu, total=%zu bytes)\n", index, total);
                
                if (!store.beginUpload(total)) {
                    uint32_t freeHeap = ESP.getFreeHeap();
                    size_t total_fs, used_fs, free_fs;
                    store.getStorageInfo(total_fs, used_fs, free_fs);
                    
                    Serial.printf("[SequencerAPI] ❌ beginUpload FAILED:\n");
                    Serial.printf("   - Heap: %u bytes free\n", freeHeap);
                    Serial.printf("   - Disk: %zu bytes free (need %zu)\n", free_fs, total);
                    Serial.printf("   - Store ready: %d\n", store.isReady() ? 1 : 0);
                    
                    request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Upload init failed - check device logs\"}");
                    return;
                }
                Serial.println("[SequencerAPI] ✅ Upload initialized successfully");
            }
            
            // Ajouter le chunk
            if (len > 0) {
                auto result = store.appendChunk(data, len);
                if (result != SequencerStoreResult::SUCCESS) {
                    Serial.printf("[SequencerAPI] ❌ appendChunk failed: %d\n", (int)result);
                    store.endUpload(false);
                    request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Append failed\"}");
                    return;
                }
                Serial.printf("[SequencerAPI] ✅ Chunk: %zu/%zu bytes\n", store.getUploadProgress(), total);
            }
            
            // Finaliser au dernier chunk
            if (index + len >= total) {
                Serial.println("[SequencerAPI] 📦 Final chunk - finalizing");
                
                auto result = store.endUpload(true);
                if (result != SequencerStoreResult::SUCCESS) {
                    Serial.printf("[SequencerAPI] ❌ endUpload failed: %d\n", (int)result);
                    request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Write failed\"}");
                    return;
                }
                
                if (reloadSequencerFromStorage()) {
                    Serial.println("[SequencerAPI] ✅ Complete!");
                }
                request->send(200, "application/json", "{\"status\":\"success\",\"size\":" + String(total) + "}");
            }
        }
    );

    // ========== Legacy endpoint /load (backward compatibility) ==========
    server.on("/api/sequencer/load", HTTP_POST, 
        [](AsyncWebServerRequest *request) {
            // Handler vide - tout est géré dans le callback
        },
        nullptr,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            Serial.printf("[SequencerAPI] 📥 /load: index=%zu, len=%zu, total=%zu\n", index, len, total);
            
            auto& store = SequencerFileStore::getInstance();
            
            if (index == 0) {
                if (!store.beginUpload(total)) {
                    request->send(400, "application/json", "{\"status\":\"error\"}");
                    return;
                }
            }
            
            if (len > 0) {
                auto result = store.appendChunk(data, len);
                if (result != SequencerStoreResult::SUCCESS) {
                    store.endUpload(false);
                    request->send(400, "application/json", "{\"status\":\"error\"}");
                    return;
                }
            }
            
            if (index + len >= total) {
                auto result = store.endUpload(true);
                if (result != SequencerStoreResult::SUCCESS) {
                    request->send(500, "application/json", "{\"status\":\"error\"}");
                    return;
                }
                
                reloadSequencerFromStorage();
                request->send(200, "application/json", "{\"status\":\"ok\"}");
            }
        }
    );

    // ========== Download endpoint ==========
    server.on("/api/sequencer/download", HTTP_GET, [](AsyncWebServerRequest *request){
        auto result = SequencerFileStore::getInstance().read();
        
        if (result.status != SequencerStoreResult::SUCCESS || result.data.empty()) {
            request->send(404, "application/json", "{\"status\":\"error\",\"message\":\"No file\"}");
            return;
        }
        
        AsyncWebServerResponse *response = request->beginResponse(200, "application/octet-stream", result.data.data(), result.data.size());
        response->addHeader("Content-Disposition", "attachment; filename=\"sequencer.nidmid\"");
        request->send(response);
        
        Serial.printf("[SequencerAPI] ✅ Downloaded %zu bytes\n", result.data.size());
    });

    // ========== View endpoint ==========
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

    // ========== Diagnostic endpoint ==========
    server.on("/api/sequencer/diagnosis", HTTP_GET, [](AsyncWebServerRequest *request){
        auto& store = SequencerFileStore::getInstance();
        
        String json = "{";
        
        // État général
        json += "\"ready\":" + String(store.isReady() ? "true" : "false") + ",";
        json += "\"uploadInProgress\":" + String(store.isUploadInProgress() ? "true" : "false") + ",";
        
        // Espace disque
        size_t total, used, free;
        store.getStorageInfo(total, used, free);
        json += "\"storage\":{";
        json += "\"total\":" + String(total) + ",";
        json += "\"used\":" + String(used) + ",";
        json += "\"free\":" + String(free);
        json += "},";
        
        // Existence fichier
        json += "\"sequenceExists\":" + String(store.sequenceExists() ? "true" : "false") + ",";
        
        // Taille fichier
        size_t seqSize = store.getSequenceSize();
        json += "\"sequenceSize\":" + String(seqSize) + ",";
        
        // Validation
        json += "\"sequenceValid\":" + String(store.validateSequence() ? "true" : "false") + ",";
        
        // LittleFS info
        json += "\"littlefs\":{";
        json += "\"mounted\":" + String(LittleFS.begin(false) ? "true" : "false") + ",";
        json += "\"dirExists\":" + String(LittleFS.exists("/seq") ? "true" : "false");
        json += "},";
        
        // Heap info
        json += "\"heap\":{";
        json += "\"free\":" + String(ESP.getFreeHeap()) + ",";
        json += "\"min\":" + String(ESP.getMinFreeHeap());
        json += "}";
        
        json += "}";
        request->send(200, "application/json", json);
    });

    // ========== Debug: Afficher contenu du fichier ==========
    server.on("/api/sequencer/file-content", HTTP_GET, [](AsyncWebServerRequest *request){
        auto& store = SequencerFileStore::getInstance();
        
        if (!store.sequenceExists()) {
            request->send(404, "application/json", "{\"error\":\"File not found\"}");
            return;
        }

        // Lire le fichier
        auto result = store.read();
        if (result.status != SequencerStoreResult::SUCCESS) {
            request->send(500, "application/json", "{\"error\":\"Failed to read file\",\"code\":" + String((int)result.status) + "}");
            return;
        }

        // Retourner les données en hexadécimal pour visualisation
        String hex = "";
        int maxBytes = 500;  // Limiter à 500 bytes pour ne pas surcharger
        int bytesToShow = (result.data.size() < maxBytes) ? result.data.size() : maxBytes;

        for (int i = 0; i < bytesToShow; i++) {
            if (i > 0) hex += " ";
            if (result.data[i] < 16) hex += "0";
            hex += String(result.data[i], HEX);
        }

        String json = "{";
        json += "\"size\":" + String(result.data.size()) + ",";
        json += "\"showing\":" + String(bytesToShow) + ",";
        json += "\"crc32Stored\":\"" + String(result.storedCRC32, HEX) + "\",";
        json += "\"crc32Calculated\":\"" + String(result.calculatedCRC32, HEX) + "\",";
        json += "\"checksumValid\":" + String(result.checksumValid ? "true" : "false") + ",";
        json += "\"hexContent\":\"" + hex + "\"";
        json += "}";

        request->send(200, "application/json", json);
    });
}