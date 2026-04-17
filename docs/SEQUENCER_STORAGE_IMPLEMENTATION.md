# Guide d'implémentation - Routes API Stockage Séquenceur

Ce guide montre comment implémenter les routes HTTP pour le stockage du séquenceur, intégrées dans `SequencerAPI.cpp`.

## Structure générale

Ajouter les routes de stockage dans `setupSequencerAPI()` après les routes existantes.

```cpp
#include "SequencerAPI.h"
#include "storage/SequencerFileStore.h"
#include <ArduinoJson.h>

// ... code existant ...

void setupSequencerAPI(AsyncWebServer& server) {
    // Routes existantes du séquenceur...
    server.on("/api/sequencer/load", HTTP_POST, ...);
    server.on("/api/sequencer/view", HTTP_GET, ...);
    
    // Routes de STOCKAGE du séquenceur (nouvelles)
    // ================================================
    
    // 1. Upload simple (POST /api/sequencer/upload)
    // 2. Upload progressif (POST /api/sequencer/upload-chunked)
    // 3. Download (GET /api/sequencer/download)
    // 4. Métadonnées (GET /api/sequencer/metadata)
    // 5. Validation (POST /api/sequencer/validate)
    // 6. Suppression (DELETE /api/sequencer/delete)
    // 7. Restore backup (POST /api/sequencer/restore-backup)
    // 8. Reset (POST /api/sequencer/reset)
}
```

---

## Exemple 1: POST /api/sequencer/upload (Upload simple)

```cpp
// Upload du fichier séquenceur (binaire)
server.on("/api/sequencer/upload", 
    HTTP_POST,
    [](AsyncWebServerRequest *request){
        // Handler appelé quand la requête arrive (pas de corps)
    },
    NULL,  // Pas de handler d'upload progressif ici
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t, size_t){
        // Handler body: data = buffer binaire, len = taille
        
        Serial.printf("[API] POST /api/sequencer/upload: %zu bytes\n", len);
        
        // Valider la taille
        if (len == 0 || len > 512 * 1024) {
            request->send(413, "application/json", 
                R"({"status":"error","code":"ERR_SIZE_EXCEEDED"})");
            return;
        }
        
        // Écrire le fichier de façon atomique
        SequencerStoreResult result = SequencerFileStore::getInstance().write(data, len);
        
        // Préparer la réponse JSON
        StaticJsonDocument<256> response;
        response["status"] = (result == SequencerStoreResult::SUCCESS) ? "success" : "error";
        response["message"] = SequencerFileStore::getErrorMessage(result);
        response["size"] = len;
        
        if (result == SequencerStoreResult::SUCCESS) {
            response["crc32"] = String(SequencerFileStore::getInstance().calculateSequenceCRC32(), HEX);
            
            size_t total, used, free;
            SequencerFileStore::getInstance().getStorageInfo(total, used, free);
            response["storage"]["total"] = total;
            response["storage"]["used"] = used;
            response["storage"]["free"] = free;
            
            request->send(200, "application/json", response.as<String>());
        } else {
            int httpStatus = SequencerFileStore::toHttpStatusCode(result);
            request->send(httpStatus, "application/json", response.as<String>());
        }
    }
);
```

---

## Exemple 2: POST /api/sequencer/upload-chunked (Upload progressif)

```cpp
// État global pour l'upload (simplifié ici, utiliser un contexte meilleur)
static size_t uploadSize = 0;

server.on("/api/sequencer/upload-chunked",
    HTTP_POST,
    [](AsyncWebServerRequest *request){
        // Handler initial
        bool begin = request->hasParam("begin");
        
        if (begin) {
            // Initialiser un nouvel upload
            if (!SequencerFileStore::getInstance().beginUpload()) {
                request->send(503, "application/json", 
                    R"({"status":"error","message":"upload init failed"})");
                return;
            }
            uploadSize = 0;
        }
        
        request->send(200, "application/json", 
            R"({"status":"waiting_for_data"})");
    },
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t, size_t){
        // Handler body: chunk reçu
        
        Serial.printf("[API] Chunk received: %zu bytes\n", len);
        
        SequencerStoreResult result = SequencerFileStore::getInstance().appendChunk(data, len);
        
        if (result != SequencerStoreResult::SUCCESS) {
            SequencerFileStore::getInstance().endUpload(false);  // Annuler
            request->send(SequencerFileStore::toHttpStatusCode(result), "application/json",
                R"({"status":"error"})");
            return;
        }
        
        // Vérifier si compression (commit)
        bool commit = !request->hasParam("commit") || 
                     request->getParam("commit")->value() == "true";
        
        if (commit) {
            result = SequencerFileStore::getInstance().endUpload(true);
            request->send(result == SequencerStoreResult::SUCCESS ? 200 : 500, 
                         "application/json", 
                         R"({"status":"complete"})");
        } else {
            // Continuer upload
            size_t progress = SequencerFileStore::getInstance().getUploadProgress();
            
            StaticJsonDocument<128> response;
            response["status"] = "in_progress";
            response["uploaded"] = progress;
            request->send(200, "application/json", response.as<String>());
        }
    }
);
```

---

## Exemple 3: GET /api/sequencer/download (Télécharger)

```cpp
server.on("/api/sequencer/download", HTTP_GET, 
    [](AsyncWebServerRequest *request){
        
        if (!SequencerFileStore::getInstance().sequenceExists()) {
            request->send(404, "application/json",
                R"({"status":"error","message":"not found"})");
            return;
        }
        
        SequencerReadResult readResult = SequencerFileStore::getInstance().read();
        
        if (readResult.status != SequencerStoreResult::SUCCESS) {
            request->send(500, "application/json",
                R"({"status":"error","message":"read failed"})");
            return;
        }
        
        // Envoyer les données binaires
        AsyncWebServerResponse *response = request->beginResponse(200, 
            "application/octet-stream", 
            readResult.data.size());
        response->addHeader("Content-Length", String(readResult.data.size()));
        request->send(response);
        
        // Envoyer les données (attention à la mémoire sur gros fichiers)
        // Meilleure approche: utiliser AsyncResponseStream
    }
);
```

---

## Exemple 4: GET /api/sequencer/metadata (Métadonnées)

```cpp
server.on("/api/sequencer/metadata", HTTP_GET, 
    [](AsyncWebServerRequest *request){
        
        StaticJsonDocument<512> response;
        
        bool exists = SequencerFileStore::getInstance().sequenceExists();
        response["exists"] = exists;
        
        size_t total, used, free;
        SequencerFileStore::getInstance().getStorageInfo(total, used, free);
        response["storage"]["total"] = total;
        response["storage"]["used"] = used;
        response["storage"]["free"] = free;
        
        if (exists) {
            response["size"] = (int)SequencerFileStore::getInstance().getSequenceSize();
            response["valid"] = SequencerFileStore::getInstance().validateSequence();
            response["crc32"] = String(SequencerFileStore::getInstance().calculateSequenceCRC32(), HEX);
        }
        
        request->send(200, "application/json", response.as<String>());
    }
);
```

---

## Exemple 5: DELETE /api/sequencer/delete (Suppression)

```cpp
server.on("/api/sequencer/delete", HTTP_DELETE,
    [](AsyncWebServerRequest *request){
        
        SequencerStoreResult result = SequencerFileStore::getInstance().deleteSequence();
        
        if (result == SequencerStoreResult::SUCCESS) {
            StaticJsonDocument<256> response;
            response["status"] = "success";
            response["message"] = "Séquence supprimée";
            
            size_t total, used, free;
            SequencerFileStore::getInstance().getStorageInfo(total, used, free);
            response["storage"]["total"] = total;
            response["storage"]["used"] = used;
            response["storage"]["free"] = free;
            
            request->send(200, "application/json", response.as<String>());
        } else {
            int httpStatus = SequencerFileStore::toHttpStatusCode(result);
            request->send(httpStatus, "application/json",
                String("{\"status\":\"error\",\"message\":\"") + 
                SequencerFileStore::getErrorMessage(result) + "\"}");
        }
    }
);
```

---

## Conseils d'implémentation

### 1. Gestion mémoire pour gros fichiers
```cpp
// ❌ À éviter (charge tout en RAM)
std::vector<uint8_t> data = readResult.data;

// ✅ Meilleur (transmission par streaming)
AsyncResponseStream *response = request->beginResponseStream("application/octet-stream");
response->write(data.data(), data.size());
request->send(response);
```

### 2. Error handling robuste
```cpp
auto& store = SequencerFileStore::getInstance();
auto result = store.write(data, len);

if (result != SequencerStoreResult::SUCCESS) {
    int httpCode = SequencerFileStore::toHttpStatusCode(result);
    const char* msg = SequencerFileStore::getErrorMessage(result);
    
    // Log + response
    Serial.printf("[API ERROR] %s (%d)\n", msg, httpCode);
    
    StaticJsonDocument<200> json;
    json["status"] = "error";
    json["code"] = (int)result;
    json["message"] = msg;
    
    request->send(httpCode, "application/json", json.as<String>());
}
```

### 3. Timeouts et protection
```cpp
// Timeout protégé pour upload
if (!request->hasArg("timeout")) {
    request->client()->setAckTimeout(5);  // 5s between packets
}

// Limiter la taille max
if (len > SequencerStoreConfig::MAX_SEQUENCER_SIZE) {
    request->send(413, "application/json", "too large");
    return;
}
```

### 4. Logging pour debugging
```cpp
Serial.printf("[SequencerAPI] POST /upload: %zu bytes (CRC: %08lx)\n", 
    len, SequencerFileStore::getInstance().calculateSequenceCRC32());
```

---

## État de développement

### Étapes suivantes (task 4.4)
- [ ] Implémenter les 8 routes HTTP dans SequencerAPI.cpp
- [ ] Ajouter les includes nécessaires (ArduinoJson, etc.)
- [ ] Tester avec cURL ou le navigateur
- [ ] Valider CRC32 end-to-end
- [ ] Améliorer streaming pour gros fichiers

### Dépendances requises
- ESPAsyncWebServer (déjà présent)
- ArduinoJson (pour les réponses JSON)
- SequencerFileStore (implémenté)

### Performance attendue
- Upload 512 KB: ~2-5 secondes (dépend du WiFi)
- Download 512 KB: ~2-5 secondes
- CRC32: <100 ms pour 512 KB
