#pragma once

/**
 * @file SequencerFileStore.h
 * @brief Gestion persistante des fichiers séquenceur (.nidmid)
 * 
 * Ce module fournit une API robuste pour:
 * - Upload atomique du fichier séquenceur via fichier temporaire + rename
 * - Lecture du fichier (en RAM ou par flux)
 * - Validation d'intégrité via CRC32
 * - Suppression et réinitialisation
 * - Gestion des erreurs avec recovery
 */

#include <Arduino.h>
#include <LittleFS.h>
#include <cstdint>
#include <vector>

/**
 * @brief Résultat d'une opération SequencerFileStore
 */
enum class SequencerStoreResult {
    SUCCESS = 0,
    ERR_NOT_MOUNTED,              // LittleFS non monté
    ERR_FILE_NOT_FOUND,           // Fichier n'existe pas
    ERR_SIZE_EXCEEDED,            // Fichier trop gros
    ERR_WRITE_FAILED,             // Erreur d'écriture
    ERR_READ_FAILED,              // Erreur de lecture
    ERR_INVALID_CHECKSUM,         // CRC32 invalide
    ERR_CHECKSUM_MISSING,         // En-tête manquant (pas de CRC stocké)
    ERR_INSUFFICIENT_SPACE,       // Pas assez d'espace libre
    ERR_TEMP_FILE_CREATE_FAILED,  // Impossible de créer fichier temporaire
    ERR_TEMP_FILE_RENAME_FAILED,  // Échec du rename atomique
    ERR_UPLOAD_INTERRUPTED,       // Upload interrompu
    ERR_UNKNOWN                   // Erreur inconnue
};

/**
 * @brief En-tête de fichier séquenceur (optionnel, pour validation)
 */
struct SequencerFileHeader {
    static constexpr uint32_t MAGIC = 0x4E504D53;  // "NPMS" en little-endian (NiDMI Pattern/Midi Sequencer)
    static constexpr size_t HEADER_SIZE = 16;
    
    uint32_t magic;               // 0x4E504D53 (NPMS)
    uint32_t version;             // Version du format (ex: 1)
    uint32_t dataSize;            // Taille des données (sans en-tête)
    uint32_t crc32;               // CRC32 des données
};

/**
 * @brief Configuration du magasin de fichiers séquenceur
 */
namespace SequencerStoreConfig {
    // Chemins
    constexpr const char* SEQUENCER_DIR = "/seq";
    constexpr const char* SEQUENCER_FILE = "/seq/nidmid.bin";
    constexpr const char* SEQUENCER_TEMP = "/seq/nidmid.tmp";
    constexpr const char* SEQUENCER_BACKUP = "/seq/nidmid.bak";
    
    // Limites de taille
    constexpr size_t MAX_SEQUENCER_SIZE = 512 * 1024;      // 512 KB max
    constexpr size_t UPLOAD_CHUNK_SIZE = 4096;             // 4 KB par chunk
    constexpr size_t MIN_SEQUENCER_SIZE = 64;              // 64 bytes min (santé minimale)
    
    // Timeouts et délais
    constexpr unsigned long UPLOAD_TIMEOUT = 120000;       // 120 s pour upload complet
    constexpr unsigned long WRITE_TIMEOUT = 10000;         // 10 s par écriture
    
    // Paramètres CRC32
    constexpr bool ENABLE_CRC32_VALIDATION = true;
    constexpr bool STORE_HEADER_WITH_CRC = true;           // Stocker en-tête + CRC avec données
    
    // Backup et recovery
    constexpr bool AUTO_BACKUP = true;                     // Créer .bak avant write
    constexpr bool AUTO_RECOVERY = true;                   // Récupérer depuis .bak si main échoue
};

/**
 * @brief Résultat d'une lecture avec métadonnées
 */
struct SequencerReadResult {
    SequencerStoreResult status;
    std::vector<uint8_t> data;
    uint32_t storedCRC32;         // CRC32 lu depuis l'en-tête
    uint32_t calculatedCRC32;     // CRC32 calculé après lecture
    size_t dataSize;
    bool headerValid;             // En-tête valide
    bool checksumValid;           // CRC32 valide
};

/**
 * @class SequencerFileStore
 * @brief Gestionnaire singleton pour la persistance des séquences
 * 
 * Stratégies d'utilisation:
 * 1. Upload atomique: Écrire dans .tmp, puis rename atomique vers .bin
 * 2. CRC32: Stocker CRC32 dans en-tête, valider à la lecture
 * 3. Backup: Créer .bak avant modification, restaurer en cas d'erreur
 * 4. Recovery: Si .bin manque, chercher dans .bak
 */
class SequencerFileStore {
public:
    /**
     * @brief Instance singleton
     */
    static SequencerFileStore& getInstance() {
        static SequencerFileStore instance;
        return instance;
    }

    /**
     * @brief Initialiser le magasin (créer répertoires, vérifier espace)
     * @return true si succès
     */
    bool begin();

    /**
     * @brief Vérifier si LittleFS est monté et prêt
     */
    bool isReady() const { return m_isReady; }

    /**
     * @brief Obtenir les informations d'espace disque
     * @param[out] total Taille totale
     * @param[out] used Espace utilisé
     * @param[out] free Espace libre
     */
    void getStorageInfo(size_t& total, size_t& used, size_t& free) const;

    /**
     * @brief Lire la séquence complète
     * @return Structure avec données, CRC32 et statut
     */
    SequencerReadResult read() const;

    /**
     * @brief Lire la séquence dans un buffer pré-alloué
     * @param buffer Buffer de destination
     * @param maxSize Taille max du buffer
     * @return Nombre d'octets lus, -1 en cas d'erreur
     */
    int readToBuffer(uint8_t* buffer, size_t maxSize);

    /**
     * @brief Écrire la séquence de façon atomique (temp + rename)
     * 
     * Stratégie:
     * 1. Créer backup du fichier existant (.bak)
     * 2. Écrire dans fichier temporaire (.tmp)
     * 3. Valider CRC32 si STORE_HEADER_WITH_CRC
     * 4. Rename atomique: .tmp → .bin
     * 5. Supprimer .tmp si existant et supprimer .bak
     * 
     * @param data Pointeur vers les données
     * @param size Taille des données
     * @return Résultat de l'opération
     */
    SequencerStoreResult write(const uint8_t* data, size_t size);

    /**
     * @brief Écrire la séquence par chunks (progressif pour gros fichiers)
     * 
     * Utile pour upload HTTP progressif. Le contexte d'upload est interne.
     * 
     * Séquence:
     * 1. Appeler beginUpload() pour initialiser
     * 2. Appeler appendChunk() plusieurs fois
     * 3. Appeler endUpload(true) pour finaliser ou endUpload(false) pour annuler
     * 
     * @param data Pointeur vers le chunk
     * @param size Taille du chunk
     * @return Résultat de l'opération
     */
    SequencerStoreResult appendChunk(const uint8_t* data, size_t size);

    /**
     * @brief Commencer un upload progressif
     * @param size Taille totale attendue du fichier (optionnel, pour pré-allouer)
     * @return true si succès
     */
    bool beginUpload(size_t size = 0);

    /**
     * @brief Terminer un upload progressif
     * @param commit true pour sauvegarder, false pour abandonner
     * @return Résultat de l'opération
     */
    SequencerStoreResult endUpload(bool commit = true);

    /**
     * @brief Obtenir la position actuelle dans l'upload
     */
    size_t getUploadProgress() const { return m_uploadProgress; }

    /**
     * @brief Vérifier si un upload est en cours
     */
    bool isUploadInProgress() const { return m_uploadInProgress; }

    /**
     * @brief Supprimer la séquence (fichier .bin)
     * @return Résultat de l'opération
     */
    SequencerStoreResult deleteSequence();

    /**
     * @brief Réinitialiser le magasin (supprimer tout: .bin, .tmp, .bak)
     * @return Résultat de l'opération
     */
    SequencerStoreResult reset();

    /**
     * @brief Vérifier l'existence du fichier séquenceur
     */
    bool sequenceExists() const;

    /**
     * @brief Obtenir la taille du fichier séquenceur
     * @return Taille en octets, 0 si absent
     */
    size_t getSequenceSize() const;

    /**
     * @brief Valider l'intégrité du fichier via CRC32
     * @return true si valide (ou si CRC32 désactivé)
     */
    bool validateSequence() const;

    /**
     * @brief Calculer le CRC32 du fichier séquenceur
     * @return CRC32 ou 0 en cas d'erreur
     */
    uint32_t calculateSequenceCRC32() const;

    /**
     * @brief Restaurer depuis le backup si disponible
     * @return Résultat de l'opération
     */
    SequencerStoreResult restoreFromBackup();

    /**
     * @brief Obtenir le message d'erreur pour un code de résultat
     */
    static const char* getErrorMessage(SequencerStoreResult result);

    /**
     * @brief Convertir SequencerStoreResult en code HTTP
     */
    static int toHttpStatusCode(SequencerStoreResult result);

    /**
     * @brief Déboguer: afficher l'état du magasin sur Serial
     */
    void debugPrintStatus() const;

private:
    SequencerFileStore() 
        : m_isReady(false), 
          m_uploadInProgress(false), 
          m_uploadProgress(0),
          m_uploadFile(),
          m_uploadMaxSize(0) {}
    
    ~SequencerFileStore();
    
    // Interdire la copie
    SequencerFileStore(const SequencerFileStore&) = delete;
    SequencerFileStore& operator=(const SequencerFileStore&) = delete;

    bool m_isReady;
    
    // État d'upload progressif (streaming direct vers fichier, pas de buffer RAM)
    bool m_uploadInProgress;
    size_t m_uploadProgress;
    File m_uploadFile;
    size_t m_uploadMaxSize;        // Taille max attendue (pour validation)
    uint32_t m_uploadCRC32;
    unsigned long m_uploadStartTime;

    /**
     * @brief Calculer CRC32 sur un buffer
     */
    static uint32_t calculateCRC32(const uint8_t* data, size_t size, uint32_t crc = 0xFFFFFFFF);

    /**
     * @brief Créer l'en-tête avec CRC32
     */
    SequencerFileHeader createHeader(size_t dataSize, uint32_t crc32);

    /**
     * @brief Vérifier l'en-tête
     */
    bool verifyHeader(const SequencerFileHeader& header) const;

    /**
     * @brief Allocer et initialiser le buffer d'upload
     */
    bool allocateUploadBuffer(size_t size);

    /**
     * @brief Nettoyer le buffer d'upload
     */
    void freeUploadBuffer();

    /**
     * @brief Opération atomique de rename (temp → final)
     */
    SequencerStoreResult atomicRename(const char* tempPath, const char* finalPath);

    /**
     * @brief Supprimer un fichier silencieusement
     */
    void silentDelete(const char* path);
};

/**
 * @brief Récupérer l'instance singleton
 */
inline SequencerFileStore& getSequencerStore() {
    return SequencerFileStore::getInstance();
}
