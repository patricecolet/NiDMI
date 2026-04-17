#include "SequencerFileStore.h"
#include "LittleFS.h"
#include <algorithm>
#include <cstring>

/**
 * @brief Table de lookup CRC32 (optimisation pour calcul rapide)
 */
static uint32_t crc32_table[256];

/**
 * @brief Initialiser la table CRC32
 */
static void initializeCRC32Table() {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320UL : 0);
        }
        crc32_table[i] = crc;
    }
}

// Initialisation statique
static bool crc32_initialized = false;

// ============================================================================
// Implémentation de SequencerFileStore
// ============================================================================

bool SequencerFileStore::begin() {
    if (!crc32_initialized) {
        initializeCRC32Table();
        crc32_initialized = true;
    }

    // Vérifier que LittleFS est monté
    if (!LittleFS.exists("/")) {
        Serial.println("[SequencerFileStore] ❌ LittleFS non monté");
        m_isReady = false;
        return false;
    }

    Serial.println("[SequencerFileStore] ✅ LittleFS est monté");

    // Créer le répertoire /seq s'il n'existe pas
    if (!LittleFS.exists(SequencerStoreConfig::SEQUENCER_DIR)) {
        Serial.printf("[SequencerFileStore] 📁 Creating directory: %s\n", SequencerStoreConfig::SEQUENCER_DIR);
        if (!LittleFS.mkdir(SequencerStoreConfig::SEQUENCER_DIR)) {
            Serial.printf("[SequencerFileStore] ❌ Impossible de créer le répertoire %s\n", SequencerStoreConfig::SEQUENCER_DIR);
            m_isReady = false;
            return false;
        }
        Serial.printf("[SequencerFileStore] ✅ Répertoire créé: %s\n", SequencerStoreConfig::SEQUENCER_DIR);
    } else {
        Serial.printf("[SequencerFileStore] ✅ Directory exists: %s\n", SequencerStoreConfig::SEQUENCER_DIR);
    }

    // Vérifier l'espace disponible
    size_t total, used, free;
    getStorageInfo(total, used, free);
    
    Serial.printf("[SequencerFileStore] 📊 Storage status:\n");
    Serial.printf("   - Total: %zu bytes\n", total);
    Serial.printf("   - Used: %zu bytes\n", used);
    Serial.printf("   - Free: %zu bytes\n", free);
    
    if (free < SequencerStoreConfig::MAX_SEQUENCER_SIZE) {
        Serial.printf("[SequencerFileStore] ⚠️  Warning: Free space (%zu) < MAX_SEQUENCER_SIZE (%zu)\n", 
            free, SequencerStoreConfig::MAX_SEQUENCER_SIZE);
    }

    // Vérifier si recovery depuis backup est nécessaire
    if (SequencerStoreConfig::AUTO_RECOVERY) {
        bool hasMain = LittleFS.exists(SequencerStoreConfig::SEQUENCER_FILE);
        bool hasBackup = LittleFS.exists(SequencerStoreConfig::SEQUENCER_BACKUP);
        
        if (!hasMain && hasBackup) {
            Serial.println("[SequencerFileStore] ℹ️  Fichier principal manquant, tentative de recovery depuis backup...");
            restoreFromBackup();
        }
    }

    m_isReady = true;
    Serial.println("[SequencerFileStore] ✅ Magasin séquenceur prêt");
    return true;
}

void SequencerFileStore::getStorageInfo(size_t& total, size_t& used, size_t& free) const {
    total = LittleFS.totalBytes();
    used = LittleFS.usedBytes();
    free = total - used;
}

SequencerReadResult SequencerFileStore::read() const {
    SequencerReadResult result = {};
    result.status = SequencerStoreResult::SUCCESS;
    result.headerValid = false;
    result.checksumValid = false;

    if (!m_isReady) {
        result.status = SequencerStoreResult::ERR_NOT_MOUNTED;
        return result;
    }

    if (!sequenceExists()) {
        result.status = SequencerStoreResult::ERR_FILE_NOT_FOUND;
        return result;
    }

    File file = LittleFS.open(SequencerStoreConfig::SEQUENCER_FILE, "r");
    if (!file) {
        result.status = SequencerStoreResult::ERR_READ_FAILED;
        return result;
    }

    size_t fileSize = file.size();

    // Lire l'en-tête si CRC32 est activé
    if (SequencerStoreConfig::STORE_HEADER_WITH_CRC) {
        if (fileSize < SequencerFileHeader::HEADER_SIZE) {
            file.close();
            result.status = SequencerStoreResult::ERR_READ_FAILED;
            return result;
        }

        SequencerFileHeader header = {};
        if (file.read((uint8_t*)&header, SequencerFileHeader::HEADER_SIZE) != SequencerFileHeader::HEADER_SIZE) {
            file.close();
            result.status = SequencerStoreResult::ERR_READ_FAILED;
            return result;
        }

        result.headerValid = verifyHeader(header);
        result.storedCRC32 = header.crc32;
        result.dataSize = header.dataSize;
    } else {
        result.dataSize = fileSize;
    }

    // Lire les données
    size_t dataStart = SequencerStoreConfig::STORE_HEADER_WITH_CRC ? SequencerFileHeader::HEADER_SIZE : 0;
    size_t dataToRead = fileSize - dataStart;

    if (dataToRead > SequencerStoreConfig::MAX_SEQUENCER_SIZE) {
        file.close();
        result.status = SequencerStoreResult::ERR_SIZE_EXCEEDED;
        return result;
    }

    result.data.resize(dataToRead);
    if (file.read(result.data.data(), dataToRead) != (int)dataToRead) {
        file.close();
        result.status = SequencerStoreResult::ERR_READ_FAILED;
        return result;
    }

    file.close();

    // Valider CRC32 si activé
    if (SequencerStoreConfig::ENABLE_CRC32_VALIDATION && SequencerStoreConfig::STORE_HEADER_WITH_CRC) {
        result.calculatedCRC32 = calculateCRC32(result.data.data(), result.data.size());
        result.checksumValid = (result.calculatedCRC32 == result.storedCRC32);
        
        if (!result.checksumValid) {
            result.status = SequencerStoreResult::ERR_INVALID_CHECKSUM;
        }
    } else {
        result.checksumValid = true;
    }

    return result;
}

int SequencerFileStore::readToBuffer(uint8_t* buffer, size_t maxSize) {
    if (!m_isReady || !buffer) {
        return -1;
    }

    if (!sequenceExists()) {
        return -1;
    }

    File file = LittleFS.open(SequencerStoreConfig::SEQUENCER_FILE, "r");
    if (!file) {
        return -1;
    }

    size_t fileSize = file.size();
    size_t dataStart = SequencerStoreConfig::STORE_HEADER_WITH_CRC ? SequencerFileHeader::HEADER_SIZE : 0;
    size_t dataSize = fileSize - dataStart;

    if (dataSize > maxSize) {
        file.close();
        return -1;
    }

    if (SequencerStoreConfig::STORE_HEADER_WITH_CRC) {
        file.seek(SequencerFileHeader::HEADER_SIZE);
    }

    int bytesRead = file.read(buffer, dataSize);
    file.close();

    return bytesRead;
}

SequencerStoreResult SequencerFileStore::write(const uint8_t* data, size_t size) {
    if (!m_isReady) {
        return SequencerStoreResult::ERR_NOT_MOUNTED;
    }

    if (!data || size == 0) {
        return SequencerStoreResult::ERR_WRITE_FAILED;
    }

    if (size > SequencerStoreConfig::MAX_SEQUENCER_SIZE) {
        return SequencerStoreResult::ERR_SIZE_EXCEEDED;
    }

    // Vérifier l'espace disponible
    size_t total, used, free;
    getStorageInfo(total, used, free);
    
    size_t requiredSpace = size;
    if (SequencerStoreConfig::STORE_HEADER_WITH_CRC) {
        requiredSpace += SequencerFileHeader::HEADER_SIZE;
    }

    if (free < requiredSpace) {
        return SequencerStoreResult::ERR_INSUFFICIENT_SPACE;
    }

    // Étape 1: créer backup du fichier existant
    if (SequencerStoreConfig::AUTO_BACKUP && sequenceExists()) {
        silentDelete(SequencerStoreConfig::SEQUENCER_BACKUP);
        if (!LittleFS.rename(SequencerStoreConfig::SEQUENCER_FILE, SequencerStoreConfig::SEQUENCER_BACKUP)) {
            Serial.println("[SequencerFileStore] ⚠️  Impossible de créer le backup");
            // Continuer malgré tout
        }
    }

    // Étape 2: créer et écrire le fichier temporaire
    File tempFile = LittleFS.open(SequencerStoreConfig::SEQUENCER_TEMP, "w");
    if (!tempFile) {
        // Recovery: restaurer depuis backup
        if (LittleFS.exists(SequencerStoreConfig::SEQUENCER_BACKUP)) {
            LittleFS.rename(SequencerStoreConfig::SEQUENCER_BACKUP, SequencerStoreConfig::SEQUENCER_FILE);
        }
        return SequencerStoreResult::ERR_TEMP_FILE_CREATE_FAILED;
    }

    // Écrire l'en-tête si CRC32 est activé
    if (SequencerStoreConfig::STORE_HEADER_WITH_CRC) {
        uint32_t dataCRC32 = calculateCRC32(data, size);
        SequencerFileHeader header = createHeader(size, dataCRC32);
        
        if (tempFile.write((const uint8_t*)&header, SequencerFileHeader::HEADER_SIZE) != SequencerFileHeader::HEADER_SIZE) {
            tempFile.close();
            silentDelete(SequencerStoreConfig::SEQUENCER_TEMP);
            if (LittleFS.exists(SequencerStoreConfig::SEQUENCER_BACKUP)) {
                LittleFS.rename(SequencerStoreConfig::SEQUENCER_BACKUP, SequencerStoreConfig::SEQUENCER_FILE);
            }
            return SequencerStoreResult::ERR_WRITE_FAILED;
        }
    }

    // Écrire les données
    if (tempFile.write(data, size) != (int)size) {
        tempFile.close();
        silentDelete(SequencerStoreConfig::SEQUENCER_TEMP);
        if (LittleFS.exists(SequencerStoreConfig::SEQUENCER_BACKUP)) {
            LittleFS.rename(SequencerStoreConfig::SEQUENCER_BACKUP, SequencerStoreConfig::SEQUENCER_FILE);
        }
        return SequencerStoreResult::ERR_WRITE_FAILED;
    }

    tempFile.flush();
    tempFile.close();

    // Étape 3: rename atomique
    SequencerStoreResult renameResult = atomicRename(SequencerStoreConfig::SEQUENCER_TEMP, SequencerStoreConfig::SEQUENCER_FILE);
    if (renameResult != SequencerStoreResult::SUCCESS) {
        if (LittleFS.exists(SequencerStoreConfig::SEQUENCER_BACKUP)) {
            LittleFS.rename(SequencerStoreConfig::SEQUENCER_BACKUP, SequencerStoreConfig::SEQUENCER_FILE);
        }
        return renameResult;
    }

    // Étape 4: nettoyer les fichiers temporaires
    silentDelete(SequencerStoreConfig::SEQUENCER_BACKUP);
    silentDelete(SequencerStoreConfig::SEQUENCER_TEMP);

    Serial.printf("[SequencerFileStore] ✅ Séquence écrite: %zu bytes\n", size);
    return SequencerStoreResult::SUCCESS;
}

bool SequencerFileStore::beginUpload(size_t size) {
    Serial.printf("[SequencerFileStore] 🔄 beginUpload() called with size=%zu\n", size);
    
    if (!m_isReady) {
        Serial.println("[SequencerFileStore] ❌ Store not ready - call begin() first!");
        return false;
    }

    if (m_uploadInProgress) {
        Serial.println("[SequencerFileStore] ⚠️  Upload already in progress");
        return false;
    }

    // Valider la taille (cap à MAX_SEQUENCER_SIZE)
    size_t maxSize = (size > 0) ? size : SequencerStoreConfig::MAX_SEQUENCER_SIZE;
    if (maxSize > SequencerStoreConfig::MAX_SEQUENCER_SIZE) {
        maxSize = SequencerStoreConfig::MAX_SEQUENCER_SIZE;
    }

    // Vérifier l'espace disponible
    size_t total, used, free;
    getStorageInfo(total, used, free);
    Serial.printf("[SequencerFileStore] 📊 Disk space: %zuB free, %zuB used, %zuB total\n", free, used, total);
    
    if (free < maxSize + 1024) {  // +1KB buffer pour sécurité
        Serial.printf("[SequencerFileStore] ❌ Insufficient disk space: need %zu, have %zu\n", maxSize, free);
        return false;
    }

    // Nettoyer les fichiers temporaires
    silentDelete(SequencerStoreConfig::SEQUENCER_TEMP);
    silentDelete(SequencerStoreConfig::SEQUENCER_BACKUP);

    // Ouvrir le fichier temporaire pour streaming
    Serial.printf("[SequencerFileStore] 📝 Opening temp file: %s\n", SequencerStoreConfig::SEQUENCER_TEMP);
    m_uploadFile = LittleFS.open(SequencerStoreConfig::SEQUENCER_TEMP, "w");
    if (!m_uploadFile) {
        Serial.printf("[SequencerFileStore] ❌ FAILED to open temp file: %s\n", SequencerStoreConfig::SEQUENCER_TEMP);
        Serial.printf("    Reason: LittleFS may not be mounted, or path invalid\n");
        return false;
    }

    m_uploadInProgress = true;
    m_uploadProgress = 0;
    m_uploadMaxSize = maxSize;
    m_uploadCRC32 = 0xFFFFFFFF;
    m_uploadStartTime = millis();

    Serial.printf("[SequencerFileStore] ✅ Upload started (streaming to %s), max size: %zu bytes\n", 
        SequencerStoreConfig::SEQUENCER_TEMP, m_uploadMaxSize);
    return true;
}

SequencerStoreResult SequencerFileStore::appendChunk(const uint8_t* data, size_t size) {
    // Vérifier l'état d'upload
    if (!m_uploadInProgress || !m_uploadFile) {
        Serial.printf("[SequencerFileStore] ❌ appendChunk failed: not in upload mode (inProgress=%d, file.size=%zu)\n", 
            (int)m_uploadInProgress, (m_uploadFile ? 1 : 0));
        return SequencerStoreResult::ERR_UPLOAD_INTERRUPTED;
    }

    if (!data || size == 0) {
        Serial.printf("[SequencerFileStore] ℹ️  Empty chunk received (skipped)\n");
        return SequencerStoreResult::SUCCESS;
    }

    // Vérifier le timeout
    unsigned long elapsed = millis() - m_uploadStartTime;
    if (elapsed > SequencerStoreConfig::UPLOAD_TIMEOUT) {
        Serial.printf("[SequencerFileStore] ❌ Upload timeout: %lu ms > %lu ms\n", 
            elapsed, SequencerStoreConfig::UPLOAD_TIMEOUT);
        endUpload(false);
        return SequencerStoreResult::ERR_UPLOAD_INTERRUPTED;
    }

    // Vérifier la taille totale
    if (m_uploadProgress + size > m_uploadMaxSize) {
        Serial.printf("[SequencerFileStore] ❌ File too large: %zu + %zu > %zu\n", 
            m_uploadProgress, size, m_uploadMaxSize);
        endUpload(false);
        return SequencerStoreResult::ERR_SIZE_EXCEEDED;
    }

    // Afficher l'état heap avant écriture pour debug
    uint32_t freeHeap = ESP.getFreeHeap();
    if (m_uploadProgress == 0) {
        Serial.printf("[SequencerFileStore] 📊 Starting chunk write (free heap: %u bytes)\n", freeHeap);
    }

    // Écrire directement au fichier
    size_t written = m_uploadFile.write(data, size);
    if (written != size) {
        Serial.printf("[SequencerFileStore] ❌ Write FAILED: %zu of %zu bytes written\n", written, size);
        Serial.printf("   - Update progress: %zu bytes\n", m_uploadProgress);
        Serial.printf("   - Chunk size: %zu bytes\n", size);
        Serial.printf("   - Free heap: %u bytes\n", ESP.getFreeHeap());
        endUpload(false);
        return SequencerStoreResult::ERR_WRITE_FAILED;
    }

    m_uploadProgress += size;

    // Mettre à jour CRC32
    m_uploadCRC32 = calculateCRC32(data, size, m_uploadCRC32);

    // ⭐ CRITICAL: Yield pour laisser l'ESP32 traiter les interruptions et les autres tâches
    // Sinon, le watchdog timer reboote l'appareil pendant les gros uploads
    yield();

    // Log toutes les 10 chunks pour éviter flood Serial
    if (m_uploadProgress % (10 * 4096) == 0 || m_uploadProgress + size >= m_uploadMaxSize) {
        Serial.printf("[SequencerFileStore] ✅ Chunk written: %zu bytes (progress: %zu/%zu, heap: %u)\n", 
            size, m_uploadProgress, m_uploadMaxSize, ESP.getFreeHeap());
    }

    return SequencerStoreResult::SUCCESS;
}

SequencerStoreResult SequencerFileStore::endUpload(bool commit) {
    if (!m_uploadInProgress) {
        return SequencerStoreResult::ERR_UPLOAD_INTERRUPTED;
    }

    SequencerStoreResult result = SequencerStoreResult::SUCCESS;

    // Fermer le fichier
    if (m_uploadFile) {
        m_uploadFile.close();
    }

    if (commit && m_uploadProgress > 0) {
        // Créer backup du fichier existant si présent
        if (SequencerStoreConfig::AUTO_BACKUP && sequenceExists()) {
            silentDelete(SequencerStoreConfig::SEQUENCER_BACKUP);
            if (!LittleFS.rename(SequencerStoreConfig::SEQUENCER_FILE, SequencerStoreConfig::SEQUENCER_BACKUP)) {
                Serial.println("[SequencerFileStore] ⚠️  Failed to backup existing file");
            }
        }

        // Finalize CRC32 and create header if needed
        if (SequencerStoreConfig::STORE_HEADER_WITH_CRC) {
            Serial.println("[SequencerFileStore] 🔄 Adding CRC32 header to file...");
            
            // Note: m_uploadCRC32 est déjà finalisé via calculateCRC32() qui retourne: crc ^ 0xFFFFFFFF
            // Ne pas refaire ^= 0xFFFFFFFF ici (ça annulerait le résultat!)
            
            // Créer l'en-tête
            SequencerFileHeader header = createHeader(m_uploadProgress, m_uploadCRC32);
            
            // Ouvrir le fichier final pour écriture (y compris en-tête)
            File finalFile = LittleFS.open(SequencerStoreConfig::SEQUENCER_FILE, "w");
            if (!finalFile) {
                Serial.println("[SequencerFileStore] ❌ Failed to create final file");
                silentDelete(SequencerStoreConfig::SEQUENCER_TEMP);
                result = SequencerStoreResult::ERR_WRITE_FAILED;
            } else {
                // Écrire l'en-tête (16 bytes)
                size_t headerWritten = finalFile.write((uint8_t*)&header, sizeof(SequencerFileHeader));
                
                // Copier le contenu du fichier temporaire
                File tempFile = LittleFS.open(SequencerStoreConfig::SEQUENCER_TEMP, "r");
                if (tempFile) {
                    uint8_t buffer[512];
                    size_t bytesRead;
                    while ((bytesRead = tempFile.read(buffer, sizeof(buffer))) > 0) {
                        size_t bytesWritten = finalFile.write(buffer, bytesRead);
                        if (bytesWritten != bytesRead) {
                            Serial.printf("[SequencerFileStore] ❌ Write error during header finalize\n");
                            result = SequencerStoreResult::ERR_WRITE_FAILED;
                            break;
                        }
                    }
                    tempFile.close();
                } else {
                    Serial.println("[SequencerFileStore] ❌ Failed to read temp file");
                    result = SequencerStoreResult::ERR_READ_FAILED;
                }
                
                finalFile.close();
                silentDelete(SequencerStoreConfig::SEQUENCER_TEMP);
                
                if (result == SequencerStoreResult::SUCCESS) {
                    Serial.printf("[SequencerFileStore] ✅ Header added (CRC32: 0x%08X)\n", m_uploadCRC32);
                }
            }
        } else {
            // Sans en-tête, renommer simplement le fichier temp
            result = atomicRename(SequencerStoreConfig::SEQUENCER_TEMP, SequencerStoreConfig::SEQUENCER_FILE);
        }
    } else if (m_uploadFile) {
        // Annulation - nettoyer le fichier temporaire
        silentDelete(SequencerStoreConfig::SEQUENCER_TEMP);
    }

    // Nettoyer l'état d'upload
    m_uploadInProgress = false;
    m_uploadProgress = 0;
    m_uploadFile = File();
    m_uploadMaxSize = 0;
    m_uploadCRC32 = 0xFFFFFFFF;

    if (commit) {
        Serial.printf("[SequencerFileStore] %s Upload finalisé (size: %zu bytes)\n", 
            result == SequencerStoreResult::SUCCESS ? "✅" : "❌", m_uploadProgress);
    } else {
        Serial.println("[SequencerFileStore] ℹ️  Upload annulé");
    }

    return result;
}

SequencerStoreResult SequencerFileStore::deleteSequence() {
    if (!m_isReady) {
        return SequencerStoreResult::ERR_NOT_MOUNTED;
    }

    if (!sequenceExists()) {
        return SequencerStoreResult::ERR_FILE_NOT_FOUND;
    }

    if (!LittleFS.remove(SequencerStoreConfig::SEQUENCER_FILE)) {
        return SequencerStoreResult::ERR_WRITE_FAILED;
    }

    Serial.println("[SequencerFileStore] ✅ Séquence supprimée");
    return SequencerStoreResult::SUCCESS;
}

SequencerStoreResult SequencerFileStore::reset() {
    if (!m_isReady) {
        return SequencerStoreResult::ERR_NOT_MOUNTED;
    }

    silentDelete(SequencerStoreConfig::SEQUENCER_FILE);
    silentDelete(SequencerStoreConfig::SEQUENCER_TEMP);
    silentDelete(SequencerStoreConfig::SEQUENCER_BACKUP);

    Serial.println("[SequencerFileStore] ✅ Magasin réinitialisé");
    return SequencerStoreResult::SUCCESS;
}

bool SequencerFileStore::sequenceExists() const {
    return LittleFS.exists(SequencerStoreConfig::SEQUENCER_FILE);
}

size_t SequencerFileStore::getSequenceSize() const {
    if (!sequenceExists()) {
        return 0;
    }
    
    File file = LittleFS.open(SequencerStoreConfig::SEQUENCER_FILE, "r");
    if (!file) {
        return 0;
    }

    size_t size = file.size();
    file.close();

    // Retourner la taille des données (sans en-tête)
    if (SequencerStoreConfig::STORE_HEADER_WITH_CRC) {
        return size > SequencerFileHeader::HEADER_SIZE ? size - SequencerFileHeader::HEADER_SIZE : 0;
    }

    return size;
}

bool SequencerFileStore::validateSequence() const {
    if (!sequenceExists()) {
        return false;
    }

    if (!SequencerStoreConfig::ENABLE_CRC32_VALIDATION) {
        return true;  // Validation désactivée = OK
    }

    SequencerReadResult result = read();
    return result.checksumValid;
}

uint32_t SequencerFileStore::calculateSequenceCRC32() const {
    if (!sequenceExists()) {
        return 0;
    }

    SequencerReadResult result = read();
    if (result.status != SequencerStoreResult::SUCCESS) {
        return 0;
    }

    return calculateCRC32(result.data.data(), result.data.size());
}

SequencerStoreResult SequencerFileStore::restoreFromBackup() {
    if (!LittleFS.exists(SequencerStoreConfig::SEQUENCER_BACKUP)) {
        return SequencerStoreResult::ERR_FILE_NOT_FOUND;
    }

    if (!LittleFS.rename(SequencerStoreConfig::SEQUENCER_BACKUP, SequencerStoreConfig::SEQUENCER_FILE)) {
        return SequencerStoreResult::ERR_WRITE_FAILED;
    }

    Serial.println("[SequencerFileStore] ✅ Restoration depuis backup réussie");
    return SequencerStoreResult::SUCCESS;
}

const char* SequencerFileStore::getErrorMessage(SequencerStoreResult result) {
    switch (result) {
        case SequencerStoreResult::SUCCESS:
            return "Succès";
        case SequencerStoreResult::ERR_NOT_MOUNTED:
            return "LittleFS non monté";
        case SequencerStoreResult::ERR_FILE_NOT_FOUND:
            return "Fichier non trouvé";
        case SequencerStoreResult::ERR_SIZE_EXCEEDED:
            return "Taille de fichier trop grande";
        case SequencerStoreResult::ERR_WRITE_FAILED:
            return "Erreur d'écriture";
        case SequencerStoreResult::ERR_READ_FAILED:
            return "Erreur de lecture";
        case SequencerStoreResult::ERR_INVALID_CHECKSUM:
            return "Checksum invalide";
        case SequencerStoreResult::ERR_CHECKSUM_MISSING:
            return "Checksum manquant";
        case SequencerStoreResult::ERR_INSUFFICIENT_SPACE:
            return "Espace disque insuffisant";
        case SequencerStoreResult::ERR_TEMP_FILE_CREATE_FAILED:
            return "Impossible de créer le fichier temporaire";
        case SequencerStoreResult::ERR_TEMP_FILE_RENAME_FAILED:
            return "Échec du rename atomique";
        case SequencerStoreResult::ERR_UPLOAD_INTERRUPTED:
            return "Upload interrompu";
        default:
            return "Erreur inconnue";
    }
}

int SequencerFileStore::toHttpStatusCode(SequencerStoreResult result) {
    switch (result) {
        case SequencerStoreResult::SUCCESS:
            return 200;
        case SequencerStoreResult::ERR_FILE_NOT_FOUND:
            return 404;
        case SequencerStoreResult::ERR_SIZE_EXCEEDED:
            return 413;  // Payload Too Large
        case SequencerStoreResult::ERR_INVALID_CHECKSUM:
        case SequencerStoreResult::ERR_CHECKSUM_MISSING:
            return 422;  // Unprocessable Entity
        case SequencerStoreResult::ERR_INSUFFICIENT_SPACE:
            return 507;  // Insufficient Storage
        case SequencerStoreResult::ERR_NOT_MOUNTED:
            return 503;  // Service Unavailable
        case SequencerStoreResult::ERR_UPLOAD_INTERRUPTED:
            return 408;  // Request Timeout
        default:
            return 500;  // Internal Server Error
    }
}

void SequencerFileStore::debugPrintStatus() const {
    Serial.println("\n[SequencerFileStore] === STATUS ===");
    Serial.printf("  Ready: %s\n", m_isReady ? "true" : "false");
    Serial.printf("  Sequence exists: %s\n", sequenceExists() ? "true" : "false");
    
    if (sequenceExists()) {
        Serial.printf("  Sequence size: %zu bytes\n", getSequenceSize());
        if (SequencerStoreConfig::ENABLE_CRC32_VALIDATION) {
            Serial.printf("  Sequence valid: %s\n", validateSequence() ? "true" : "false");
        }
    }

    size_t total, used, free;
    getStorageInfo(total, used, free);
    Serial.printf("  Storage: %zu / %zu bytes (free: %zu)\n", used, total, free);

    Serial.printf("  Upload in progress: %s\n", m_uploadInProgress ? "true" : "false");
    if (m_uploadInProgress) {
        Serial.printf("  Upload progress: %zu bytes\n", m_uploadProgress);
    }

    Serial.println("[SequencerFileStore] ==============\n");
}

// ============================================================================
// Méthodes privées
// ============================================================================

uint32_t SequencerFileStore::calculateCRC32(const uint8_t* data, size_t size, uint32_t crc) {
    if (!crc32_initialized) {
        initializeCRC32Table();
        crc32_initialized = true;
    }

    for (size_t i = 0; i < size; i++) {
        uint8_t byte = data[i];
        crc = crc32_table[(crc ^ byte) & 0xFF] ^ (crc >> 8);
    }

    return crc ^ 0xFFFFFFFF;
}

SequencerFileHeader SequencerFileStore::createHeader(size_t dataSize, uint32_t crc32) {
    SequencerFileHeader header = {};
    header.magic = SequencerFileHeader::MAGIC;
    header.version = 1;
    header.dataSize = dataSize;
    header.crc32 = crc32;
    return header;
}

bool SequencerFileStore::verifyHeader(const SequencerFileHeader& header) const {
    return header.magic == SequencerFileHeader::MAGIC && header.version == 1;
}

bool SequencerFileStore::allocateUploadBuffer(size_t size) {
    // Plus utilisé - le streaming écritsur disque directement
    (void)size;  // Éviter les avertissements
    return true;
}

void SequencerFileStore::freeUploadBuffer() {
    // Plus utilisé - le streaming écrit sur disque directement, pas de buffer RAM
}

SequencerStoreResult SequencerFileStore::atomicRename(const char* tempPath, const char* finalPath) {
    // Supprimer le fichier final s'il existe
    if (LittleFS.exists(finalPath)) {
        if (!LittleFS.remove(finalPath)) {
            return SequencerStoreResult::ERR_TEMP_FILE_RENAME_FAILED;
        }
    }

    // Renommer le fichier temporaire
    if (!LittleFS.rename(tempPath, finalPath)) {
        return SequencerStoreResult::ERR_TEMP_FILE_RENAME_FAILED;
    }

    return SequencerStoreResult::SUCCESS;
}

void SequencerFileStore::silentDelete(const char* path) {
    if (LittleFS.exists(path)) {
        LittleFS.remove(path);
    }
}

SequencerFileStore::~SequencerFileStore() {
    freeUploadBuffer();
}

// ============================================================================
// Stub pour les routes HTTP
// ============================================================================

// Note: Les routes HTTP sont enregistrées dans SequencerAPI.cpp via setupSequencerAPI()
