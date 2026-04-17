#pragma once

/**
 * @file LittleFS.h
 * @brief Gestion des fichiers LittleFS pour la persistance de données (séquenceur, configuration, etc.)
 * 
 * Ce module fournit une interface pour:
 * - Upload/download de fichiers (séquenceur .nidmid)
 * - Gestion des répertoires
 * - Métadonnées et validation
 * - Gestion d'erreurs robuste
 */

#include <Arduino.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>

/**
 * @brief Configuration par défaut pour le système de fichiers LittleFS
 */
namespace LittleFSConfig {
    // Chemins et répertoires
    constexpr const char* LITTLEFS_ROOT = "/";
    constexpr const char* SEQUENCER_DIR = "/seq";
    constexpr const char* SEQUENCER_FILE = "/seq/nidmid.bin";
    constexpr const char* MAPPING_DIR = "/map";
    constexpr const char* MAPPING_FILE = "/map/mapping.json";
    
    // Limites de taille (en octets)
    constexpr size_t MAX_SEQUENCER_FILE_SIZE = 512 * 1024;  // 512 KB max pour séquence
    constexpr size_t MAX_MAPPING_FILE_SIZE = 128 * 1024;    // 128 KB max pour mapping
    constexpr size_t MAX_FILE_SIZE_GENERAL = 256 * 1024;    // 256 KB pour fichiers génériques
    
    // Timeouts et délais
    constexpr unsigned long UPLOAD_CHUNK_SIZE = 4096;       // 4 KB par chunk
    constexpr unsigned long FILE_OPERATION_TIMEOUT = 120000;  // 120 s timeout
};

/**
 * @brief Structure pour les métadonnées de fichier
 */
struct FileMetadata {
    String path;
    size_t size;
    time_t lastModified;
    bool exists;
    uint32_t crc32;  // CRC optionnel pour validation
};

/**
 * @brief Résultat d'une opération LittleFS
 */
enum class LittleFSResult {
    SUCCESS = 0,
    ERR_NOT_MOUNTED,
    ERR_FILE_NOT_FOUND,
    ERR_FILE_EXISTS,
    ERR_DIR_EXISTS,
    ERR_SIZE_EXCEEDED,
    ERR_WRITE_FAILED,
    ERR_READ_FAILED,
    ERR_INVALID_PATH,
    ERR_DISK_FULL,
    ERR_TIMEOUT,
    ERR_UNKNOWN
};

/**
 * @class LittleFSManager
 * @brief Gestionnaire centralisé pour LittleFS
 */
class LittleFSManager {
public:
    /**
     * @brief Instance singleton
     */
    static LittleFSManager& getInstance() {
        static LittleFSManager instance;
        return instance;
    }

    /**
     * @brief Initialiser et vérifier le système de fichiers
     * @return true si succès, false sinon
     */
    bool begin();

    /**
     * @brief Vérifier si LittleFS est mounté
     */
    bool isMounted() const { return m_isMounted; }

    /**
     * @brief Obtenir les informations d'espace disque
     * @param[out] total Taille totale en octets
     * @param[out] used Espace utilisé en octets
     * @return true si succès
     */
    bool getStorageInfo(size_t& total, size_t& used);

    /**
     * @brief Lire le contenu d'un fichier
     * @param path Chemin du fichier
     * @param buffer Pointeur vers le buffer de destination
     * @param maxSize Taille maximum à lire
     * @return Nombre d'octets lus, -1 en cas d'erreur
     */
    int readFile(const char* path, uint8_t* buffer, size_t maxSize);

    /**
     * @brief Écrire le contenu d'un fichier (atomique avec fichier temporaire)
     * @param path Chemin du fichier
     * @param data Pointeur vers les données
     * @param size Taille des données
     * @return Résultat de l'opération
     */
    LittleFSResult writeFile(const char* path, const uint8_t* data, size_t size);

    /**
     * @brief Écrire le contenu d'un fichier avec synchronisation
     * @param path Chemin du fichier
     * @param data Pointeur vers les données
     * @param size Taille des données
     * @return Résultat de l'opération
     */
    LittleFSResult writeFileSync(const char* path, const uint8_t* data, size_t size);

    /**
     * @brief Supprimer un fichier
     * @param path Chemin du fichier
     * @return Résultat de l'opération
     */
    LittleFSResult deleteFile(const char* path);

    /**
     * @brief Supprimer un répertoire (récursif)
     * @param path Chemin du répertoire
     * @return Résultat de l'opération
     */
    LittleFSResult deleteDirectory(const char* path);

    /**
     * @brief Créer un répertoire
     * @param path Chemin du répertoire
     * @return Résultat de l'opération
     */
    LittleFSResult createDirectory(const char* path);

    /**
     * @brief Vérifier si un chemin existe
     * @param path Chemin à vérifier
     * @return true si existe, false sinon
     */
    bool exists(const char* path);

    /**
     * @brief Vérifier si un chemin est un répertoire
     * @param path Chemin à vérifier
     * @return true si c'est un répertoire
     */
    bool isDirectory(const char* path);

    /**
     * @brief Obtenir les métadonnées d'un fichier
     * @param path Chemin du fichier
     * @return Structure FileMetadata
     */
    FileMetadata getFileMetadata(const char* path);

    /**
     * @brief Calculer le CRC32 d'un fichier
     * @param path Chemin du fichier
     * @return CRC32 ou 0 en cas d'erreur
     */
    uint32_t calculateFileCRC32(const char* path);

    /**
     * @brief Valider l'intégrité d'un fichier
     * @param path Chemin du fichier
     * @param expectedCRC32 CRC32 attendu
     * @return true si valide
     */
    bool validateFileCRC32(const char* path, uint32_t expectedCRC32);

    /**
     * @brief Lister les fichiers d'un répertoire
     * @param dirPath Chemin du répertoire
     * @param[out] fileList Vector des noms de fichiers
     * @return Nombre de fichiers listés
     */
    int listFiles(const char* dirPath, std::vector<String>& fileList);

    /**
     * @brief Formater LittleFS (⚠️ destructif!)
     * @return true si succès
     */
    bool format();

    /**
     * @brief Obtenir le message d'erreur pour un code LittleFSResult
     */
    const char* getErrorMessage(LittleFSResult result) const;

private:
    LittleFSManager() : m_isMounted(false) {}
    
    ~LittleFSManager() = default;
    
    // Interdire la copie
    LittleFSManager(const LittleFSManager&) = delete;
    LittleFSManager& operator=(const LittleFSManager&) = delete;

    bool m_isMounted;
    
    /**
     * @brief Nettoyer un chemin (normaliser)
     */
    String sanitizePath(const String& path) const;
    
    /**
     * @brief Valider un chemin
     */
    bool isValidPath(const String& path) const;
};

/**
 * @brief Récupérer l'instance singleton du gestionnaire LittleFS
 */
inline LittleFSManager& getLittleFSManager() {
    return LittleFSManager::getInstance();
}

/**
 * @brief Initialiser les routes HTTP pour LittleFS
 * @param server Serveur web asynchrone
 */
void setupLittleFSAPI(AsyncWebServer& server);

/**
 * @brief Utilitaires de conversion d'erreur LittleFSResult
 */
inline int littleFSResultToHttpStatus(LittleFSResult result) {
    switch (result) {
        case LittleFSResult::SUCCESS:
            return 200;
        case LittleFSResult::ERR_NOT_MOUNTED:
            return 503;  // Service Unavailable
        case LittleFSResult::ERR_FILE_NOT_FOUND:
            return 404;
        case LittleFSResult::ERR_FILE_EXISTS:
        case LittleFSResult::ERR_DIR_EXISTS:
            return 409;  // Conflict
        case LittleFSResult::ERR_SIZE_EXCEEDED:
            return 413;  // Payload Too Large
        case LittleFSResult::ERR_INVALID_PATH:
            return 400;  // Bad Request
        case LittleFSResult::ERR_DISK_FULL:
            return 507;  // Insufficient Storage
        default:
            return 500;  // Internal Server Error
    }
}
