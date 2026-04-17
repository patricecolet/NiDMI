# SequencerFileStore - API HTTP Routes

Ce document décrit les routes HTTP à implémenter pour l'API de stockage du séquenceur.

## Routes HTTP

### 1. POST /api/sequencer/upload
**Upload du fichier séquenceur**

#### Request
```
Content-Type: application/octet-stream
Body: Binary data (séquence MIDI)
```

#### Response (Success)
```json
{
  "status": "success",
  "message": "Séquence uploaded",
  "size": 1024,
  "crc32": "0xabcd1234",
  "storage": {
    "total": 524288,
    "used": 2048,
    "free": 522240
  }
}
```

#### Response (Error)
```json
{
  "status": "error",
  "code": "ERR_SIZE_EXCEEDED",
  "message": "Taille de fichier trop grande",
  "max_size": 524288
}
```

#### Status Codes
- 200: Upload réussi
- 413: Payload too large (fichier trop gros)
- 503: LittleFS not mounted
- 507: Insufficient storage
- 500: Internal error

---

### 2. POST /api/sequencer/upload-chunked
**Upload progressif par chunks (pour gros fichiers)**

#### Request
```
POST /api/sequencer/upload-chunked?begin=true&size=102400
Content-Type: application/octet-stream
```

#### Query Parameters
- `begin` (optional, default=false): Initialiser un nouvel upload
- `commit` (optional, default=true): Finaliser l'upload
- `size` (optional): Taille totale prévue (info)

#### Request Body
```
Binary chunk data
```

#### Response
```json
{
  "status": "in_progress",
  "uploaded": 4096,
  "total_size": 102400,
  "progress_percent": 4
}
```

---

### 3. GET /api/sequencer/download
**Télécharger la séquence**

#### Request
```
GET /api/sequencer/download
```

#### Response (Success)
```
Content-Type: application/octet-stream
Content-Length: 1024
Body: Binary sequencer data
```

#### Response (Not Found)
```json
{
  "status": "error",
  "code": "ERR_FILE_NOT_FOUND",
  "message": "Aucune séquence"
}
```

#### Status Codes
- 200: OK
- 404: File not found
- 503: LittleFS not mounted

---

### 4. GET /api/sequencer/metadata
**Obtenir les métadonnées du fichier séquenceur**

#### Request
```
GET /api/sequencer/metadata
```

#### Response (Success)
```json
{
  "exists": true,
  "size": 1024,
  "crc32": "0xabcd1234",
  "valid": true,
  "storage": {
    "total": 524288,
    "used": 2048,
    "free": 522240
  },
  "header": {
    "magic": "NPMS",
    "version": 1,
    "data_size": 1024
  }
}
```

#### Response (No sequencer)
```json
{
  "exists": false,
  "storage": {
    "total": 524288,
    "used": 0,
    "free": 524288
  }
}
```

---

### 5. DELETE /api/sequencer/delete
**Supprimer la séquence**

#### Request
```
DELETE /api/sequencer/delete
```

#### Response (Success)
```json
{
  "status": "success",
  "message": "Séquence supprimée",
  "storage": {
    "total": 524288,
    "used": 0,
    "free": 524288
  }
}
```

#### Response (Not Found)
```json
{
  "status": "error",
  "code": "ERR_FILE_NOT_FOUND",
  "message": "Aucune séquence"
}
```

---

### 6. POST /api/sequencer/validate
**Valider l'intégrité du fichier**

#### Request
```
POST /api/sequencer/validate
```

#### Response (Valid)
```json
{
  "status": "valid",
  "crc32_match": true,
  "stored_crc32": "0xabcd1234",
  "calculated_crc32": "0xabcd1234"
}
```

#### Response (Invalid)
```json
{
  "status": "invalid",
  "crc32_match": false,
  "stored_crc32": "0xabcd1234",
  "calculated_crc32": "0xefgh5678",
  "message": "CRC32 mismatch - fichier corrompu"
}
```

---

### 7. POST /api/sequencer/restore-backup
**Restaurer depuis la copie de secours**

#### Request
```
POST /api/sequencer/restore-backup
```

#### Response (Success)
```json
{
  "status": "success",
  "message": "Séquence restaurée depuis backup",
  "size": 1024
}
```

#### Response (No Backup)
```json
{
  "status": "error",
  "code": "ERR_FILE_NOT_FOUND",
  "message": "Aucune copie de secours disponible"
}
```

---

### 8. POST /api/sequencer/reset
**Réinitialiser le magasin (supprimer tout)**

#### Request
```
POST /api/sequencer/reset
```

#### Response
```json
{
  "status": "success",
  "message": "Magasin réinitialisé",
  "storage": {
    "total": 524288,
    "used": 0,
    "free": 524288
  }
}
```

---

## Error Codes

| Code | HTTP | Description |
|------|------|-------------|
| SUCCESS | 200 | Opération réussie |
| ERR_NOT_MOUNTED | 503 | LittleFS non monté |
| ERR_FILE_NOT_FOUND | 404 | Fichier non trouvé |
| ERR_SIZE_EXCEEDED | 413 | Taille trop grande |
| ERR_WRITE_FAILED | 500 | Erreur d'écriture |
| ERR_READ_FAILED | 500 | Erreur de lecture |
| ERR_INVALID_CHECKSUM | 422 | CRC32 invalide |
| ERR_INSUFFICIENT_SPACE | 507 | Espace disque insuffisant |
| ERR_TEMP_FILE_CREATE_FAILED | 500 | Impossible de créer fichier temporaire |
| ERR_TEMP_FILE_RENAME_FAILED | 500 | Échec du rename atomique |
| ERR_UPLOAD_INTERRUPTED | 408 | Upload interrompu |

---

## Exemples d'utilisation (cURL)

### Upload simple
```bash
curl -X POST \
  --data-binary @nidmid.bin \
  http://192.168.4.1/api/sequencer/upload
```

### Download
```bash
curl -X GET \
  http://192.168.4.1/api/sequencer/download \
  -o nidmid.bin
```

### Metadata
```bash
curl -X GET \
  http://192.168.4.1/api/sequencer/metadata \
  | jq .
```

### Upload chunked
```bash
# Initialiser
curl -X POST "http://192.168.4.1/api/sequencer/upload-chunked?begin=true&size=102400"

# Envoyer chunks
curl -X POST \
  --data-binary @chunk1.bin \
  "http://192.168.4.1/api/sequencer/upload-chunked?commit=false"

curl -X POST \
  --data-binary @chunk2.bin \
  "http://192.168.4.1/api/sequencer/upload-chunked?commit=true"
```

---

## État du développement

### À implémenter dans SequencerAPI.cpp

Routes à enregistrer dans `setupSequencerAPI()`:

1. ✅ API header structure prête (SequencerFileStore.h)
2. ✅ SequencerFileStore singleton implémenté
3. ⏳ Routes HTTP à créer:
   - POST /api/sequencer/upload
   - POST /api/sequencer/upload-chunked
   - GET /api/sequencer/download
   - GET /api/sequencer/metadata
   - DELETE /api/sequencer/delete
   - POST /api/sequencer/validate
   - POST /api/sequencer/restore-backup
   - POST /api/sequencer/reset

### Authentification

Pour l'instant (réseau local): aucune authentification requise.
À améliorer si besoin (token, header security, etc.).
