#include "SequencerPlaybackAPI.h"
#include "storage/SequencerFileStore.h"
#include "processors/SequencerProcessor.h"
#include <LittleFS.h>

// ============================================================================
// State tracking for sequencer playback
// ============================================================================

struct SequencerState {
    char currentFile[64] = "";      // Current loaded file name
    uint8_t currentMeasure = 1;     // Current measure (1-indexed)
    uint8_t stepIndex = 0;          // Current step in sequence (0-indexed)
    uint8_t totalMeasures = 0;      // Total measures in current sequence
};

static SequencerState g_playbackState;

// Forward declarations from SequencerProcessor
extern Step steps[32];
extern uint8_t stepCount;
extern void parseNidmid(uint8_t* data, size_t len);
extern bool reloadSequencerFromStorage();

// ============================================================================
// Helper functions
// ============================================================================

/**
 * @brief List all .nidmid files in a LittleFS directory
 * @return JSON array string like ["file1.nidmid","file2.nidmid"]
 */
String listNidmidFiles(const char* directory = "/seq") {
    String result = "[";
    bool first = true;
    
    File dir = LittleFS.open(directory, "r");
    if (!dir || !dir.isDirectory()) {
        Serial.printf("[SequencerPlaybackAPI] Directory not found: %s\n", directory);
        return result + "]";
    }
    
    File file = dir.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            String name = file.name();
            if (name.endsWith(".nidmid") || name.endsWith(".NIDMID")) {
                if (!first) result += ",";
                result += "\"" + name + "\"";
                first = false;
            }
        }
        file = dir.openNextFile();
    }
    
    return result + "]";
}

/**
 * @brief Calculate total measures in current sequence
 */
uint8_t calculateTotalMeasures() {
    if (stepCount == 0) return 0;
    
    uint8_t maxMeasure = 0;
    for (int i = 0; i < stepCount; i++) {
        if (steps[i].measure > maxMeasure) {
            maxMeasure = steps[i].measure;
        }
    }
    return maxMeasure;
}

/**
 * @brief Find first step in given measure
 * @return Step index, or -1 if not found
 */
int findFirstStepInMeasure(uint8_t measure) {
    for (int i = 0; i < stepCount; i++) {
        if (steps[i].measure == measure) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Get next step index after current
 * @return Next step index, or -1 if at end
 */
int getNextStepIndex() {
    if (g_playbackState.stepIndex + 1 >= stepCount) {
        return -1;  // End of sequence
    }
    return g_playbackState.stepIndex + 1;
}

// ============================================================================
// API Endpoints
// ============================================================================

void setupSequencerPlaybackAPI(AsyncWebServer& server) {
    
    // ========== GET /api/files ==========
    // Return list of available .nidmid files
    server.on("/api/files", HTTP_GET, [](AsyncWebServerRequest *request) {
        String fileList = listNidmidFiles("/seq");
        request->send(200, "application/json", fileList);
        Serial.printf("[SequencerPlaybackAPI] GET /api/files → %s\n", fileList.c_str());
    });
    
    // ========== GET /api/status ==========
    // Return current playback status
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "{";
        json += "\"file\":\"" + String(g_playbackState.currentFile) + "\",";
        json += "\"measure\":" + String(g_playbackState.currentMeasure) + ",";
        json += "\"totalMeasures\":" + String(g_playbackState.totalMeasures) + ",";
        json += "\"stepIndex\":" + String(g_playbackState.stepIndex) + ",";
        json += "\"totalSteps\":" + String(stepCount);
        json += "}";
        
        request->send(200, "application/json", json);
        Serial.printf("[SequencerPlaybackAPI] GET /api/status → measure=%d, step=%d\n", 
                      g_playbackState.currentMeasure, g_playbackState.stepIndex);
    });
    
    // ========== POST /api/select ==========
    // Load a .nidmid file
    server.on("/api/select", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!request->hasHeader("Content-Type") || 
            request->header("Content-Type").indexOf("application/json") < 0) {
            request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid Content-Type\"}");
            return;
        }
    }, nullptr,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (index + len != total) return;  // Not complete yet
        
        // Parse JSON request - simple parsing for {"file":"..."}
        String dataStr((char*)data, len);
        int filePos = dataStr.indexOf("\"file\"");
        if (filePos < 0) {
            request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
            return;
        }
        
        // Extract filename between quotes
        int startQuote = dataStr.indexOf("\"", filePos + 7);
        int endQuote = dataStr.indexOf("\"", startQuote + 1);
        if (startQuote < 0 || endQuote < 0) {
            request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
            return;
        }
        
        String fileName = dataStr.substring(startQuote + 1, endQuote);
        Serial.printf("[SequencerPlaybackAPI] POST /api/select → file=%s\n", fileName.c_str());
        
        // Load file from /seq directory
        String filePath = "/seq/" + fileName;
        
        // Read file into buffer
        File file = LittleFS.open(filePath, "r");
        if (!file || file.isDirectory()) {
            Serial.printf("[SequencerPlaybackAPI] File not found: %s\n", filePath.c_str());
            request->send(404, "application/json", "{\"ok\":false,\"error\":\"File not found\"}");
            return;
        }
        
        size_t fileSize = file.size();
        uint8_t* buffer = new uint8_t[fileSize];
        if (!buffer) {
            request->send(500, "application/json", "{\"ok\":false,\"error\":\"Out of memory\"}");
            file.close();
            return;
        }
        
        size_t bytesRead = file.read(buffer, fileSize);
        file.close();
        
        if (bytesRead != fileSize) {
            delete[] buffer;
            request->send(500, "application/json", "{\"ok\":false,\"error\":\"Read error\"}");
            return;
        }
        
        // Parse the file
        parseNidmid(buffer, fileSize);
        delete[] buffer;
        
        // Update state
        strncpy(g_playbackState.currentFile, fileName.c_str(), sizeof(g_playbackState.currentFile) - 1);
        g_playbackState.currentMeasure = 1;
        g_playbackState.stepIndex = 0;
        g_playbackState.totalMeasures = calculateTotalMeasures();
        
        // Return response
        String response = "{";
        response += "\"ok\":true,";
        response += "\"file\":\"" + String(g_playbackState.currentFile) + "\",";
        response += "\"totalMeasures\":" + String(g_playbackState.totalMeasures);
        response += "}";
        
        request->send(200, "application/json", response);
        
        Serial.printf("[SequencerPlaybackAPI] File loaded: %s (%d steps, %d measures)\n", 
                      fileName.c_str(), stepCount, g_playbackState.totalMeasures);
    });
    
    // ========== POST /api/measure ==========
    // Set current measure
    server.on("/api/measure", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!request->hasHeader("Content-Type") || 
            request->header("Content-Type").indexOf("application/json") < 0) {
            request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid Content-Type\"}");
            return;
        }
    }, nullptr,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (index + len != total) return;
        
        // Parse JSON - extract measure number
        String dataStr((char*)data, len);
        int measurePos = dataStr.indexOf("\"measure\"");
        if (measurePos < 0) {
            request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
            return;
        }
        
        // Extract number after "measure":
        int colonPos = dataStr.indexOf(":", measurePos);
        if (colonPos < 0) {
            request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
            return;
        }
        
        String numStr = "";
        for (int i = colonPos + 1; i < len && i < colonPos + 10; i++) {
            char c = dataStr[i];
            if (c >= '0' && c <= '9') {
                numStr += c;
            } else if (numStr.length() > 0) {
                break;
            }
        }
        
        uint8_t measure = numStr.toInt();
        
        // Validate measure range
        if (measure < 1 || measure > g_playbackState.totalMeasures) {
            measure = (measure < 1) ? 1 : g_playbackState.totalMeasures;
        }
        
        // Find first step in this measure
        int stepIdx = findFirstStepInMeasure(measure);
        if (stepIdx >= 0) {
            g_playbackState.stepIndex = stepIdx;
        } else {
            g_playbackState.stepIndex = 0;
        }
        
        g_playbackState.currentMeasure = measure;
        
        String response = "{";
        response += "\"ok\":true,";
        response += "\"measure\":" + String(g_playbackState.currentMeasure);
        response += "}";
        
        request->send(200, "application/json", response);
        
        Serial.printf("[SequencerPlaybackAPI] Measure changed to %d (step=%d)\n", 
                      measure, g_playbackState.stepIndex);
    });
    
    // ========== POST /api/step ==========
    // Play next step
    server.on("/api/step", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (stepCount == 0) {
            request->send(400, "application/json", "{\"ok\":false,\"error\":\"No sequence loaded\"}");
            return;
        }
        
        uint8_t stepIdx = g_playbackState.stepIndex;
        if (stepIdx >= stepCount) {
            stepIdx = 0;
            g_playbackState.stepIndex = 0;
        }
        
        Step& step = steps[stepIdx];
        
        // Get first note (or create default if no notes)
        uint8_t note = (step.noteCount > 0) ? step.notes[0].pitch : 60;
        uint8_t velocity = (step.noteCount > 0) ? step.notes[0].velocity : 64;
        
        // Update state
        g_playbackState.currentMeasure = step.measure;
        int nextIdx = getNextStepIndex();
        bool done = (nextIdx < 0);
        
        if (!done) {
            g_playbackState.stepIndex = nextIdx;
        } else {
            g_playbackState.stepIndex = 0;  // Loop back
        }
        
        // Build response
        String response = "{";
        response += "\"ok\":true,";
        response += "\"note\":" + String(note) + ",";
        response += "\"velocity\":" + String(velocity) + ",";
        response += "\"channel\":1,";
        response += "\"measure\":" + String(step.measure) + ",";
        response += "\"stepIndex\":" + String(stepIdx) + ",";
        response += "\"totalSteps\":" + String(stepCount) + ",";
        response += "\"done\":" + String(done ? "true" : "false");
        
        // Include all notes if more than one
        if (step.noteCount > 1) {
            response += ",\"notes\":[";
            for (int i = 0; i < step.noteCount; i++) {
                if (i > 0) response += ",";
                response += "{\"pitch\":" + String(step.notes[i].pitch) + ",";
                response += "\"velocity\":" + String(step.notes[i].velocity) + "}";
            }
            response += "]";
        }
        
        response += "}";
        request->send(200, "application/json", response);
        
        Serial.printf("[SequencerPlaybackAPI] Step %d: note=%d, vel=%d, measure=%d (done=%d)\n", 
                      stepIdx, note, velocity, step.measure, done);
    });
    
    // ========== POST /api/reset ==========
    // Reset playback to start
    server.on("/api/reset", HTTP_POST, [](AsyncWebServerRequest *request) {
        g_playbackState.stepIndex = 0;
        g_playbackState.currentMeasure = 1;
        
        String response = "{\"ok\":true}";
        request->send(200, "application/json", response);
        
        Serial.println("[SequencerPlaybackAPI] Sequencer reset to start");
    });
    
    // ========== POST /api/upload ==========
    // Upload a new .nidmid file
    server.on("/api/upload", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            // Handler for completion - do nothing here
        },
        [](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, 
           size_t len, bool final) {
            // Stream handler for file upload
            static File uploadFile;
            static size_t totalUploaded = 0;
            
            if (index == 0) {
                // First chunk - create file
                totalUploaded = 0;
                String filePath = "/seq/" + filename;
                uploadFile = LittleFS.open(filePath, "w");
                if (!uploadFile) {
                    Serial.printf("[SequencerPlaybackAPI] Failed to create upload file: %s\n", filePath.c_str());
                    request->send(500, "application/json", "{\"ok\":false,\"error\":\"Failed to create file\"}");
                    return;
                }
                Serial.printf("[SequencerPlaybackAPI] Starting upload: %s (free heap: %u)\n", filePath.c_str(), ESP.getFreeHeap());
            }
            
            if (uploadFile && len > 0) {
                size_t written = uploadFile.write(data, len);
                totalUploaded += written;
                if (written != len) {
                    Serial.printf("[SequencerPlaybackAPI] Write error: wrote %zu/%zu bytes (total: %zu)\n", written, len, totalUploaded);
                } else if (index % (50 * 1024) < len) { // Log every 50KB
                    Serial.printf("[SequencerPlaybackAPI] Uploaded %zu bytes, free heap: %u\n", totalUploaded, ESP.getFreeHeap());
                }
            }
            
            if (final) {
                if (uploadFile) {
                    uploadFile.close();
                    Serial.printf("[SequencerPlaybackAPI] Upload complete: %s (%zu bytes)\n", filename.c_str(), totalUploaded);
                    request->send(200, "application/json", "{\"ok\":true,\"file\":\"" + filename + "\",\"size\":" + String(totalUploaded) + "}");
                } else {
                    request->send(500, "application/json", "{\"ok\":false,\"error\":\"Upload failed\"}");
                }
                totalUploaded = 0;
            }
        }
    );
    
    Serial.println("[SequencerPlaybackAPI] Sequencer playback API initialized");
}
