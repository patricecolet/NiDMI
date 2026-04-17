# Guide d'utilisation - API Stockage Séquenceur NiDMI

## Vue d'ensemble

L'API de stockage du séquenceur fournit 8 routes HTTP pour gérer la persistance du fichier séquenceur (`.nidmid`) sur la partition LittleFS de l'ESP32.

### Authentification
- **Réseau local uniquement** (pas de token pour l'instant)
- À améliorer si besoin: ajouter token/header de sécurité

### Adresse de base
```
http://192.168.4.1  (AP WiFi NiDMI)
http://nidmi.local   (mDNS)
```

---

## Routes API

### 1. POST /api/sequencer/upload
**Upload simple du fichier séquenceur**

#### Request
```bash
curl -X POST \
  --data-binary @nidmid.bin \
  http://192.168.4.1/api/sequencer/upload
```

#### Headers
```
Content-Type: application/octet-stream
Content-Length: <taille en bytes>
```

#### Response (Success - 200)
```json
{
  "status": "success",
  "message": "Séquence uploadée",
  "size": 1024,
  "crc32": "a1b2c3d4",
  "storage": {
    "total": 524288,
    "used": 2048,
    "free": 522240
  }
}
```

#### Response (Error - 413)
```json
{
  "status": "error",
  "code": "ERR_SIZE_EXCEEDED",
  "message": "Taille de fichier trop grande",
  "max_size": 524288,
  "uploaded_size": 1048576
}
```

#### Status Codes
| Code | Signification |
|------|--------------|
| 200 | Upload réussi |
| 400 | Requête invalide (fichier vide, etc.) |
| 403 | Accès refusé |
| 413 | Payload too large (fichier trop gros) |
| 500 | Erreur interne |
| 503 | LittleFS non monté |
| 507 | Espace disque insuffisant |

---

### 2. POST /api/sequencer/upload-chunked
**Upload progressif par chunks (pour gros fichiers > 100 KB)**

#### Étape 1: Initialiser
```bash
curl -X POST "http://192.168.4.1/api/sequencer/upload-chunked?begin=true"
```

#### Étape 2: Envoyer chunks
```bash
# Chunk 1 (commit=false pour continuer)
curl -X POST \
  --data-binary @chunk1.bin \
  "http://192.168.4.1/api/sequencer/upload-chunked?commit=false"

# Response
{
  "status": "in_progress",
  "uploaded": 4096,
  "max_size": 524288,
  "progress_percent": 1
}
```

#### Étape 3: Finaliser
```bash
# Dernier chunk (commit=true par défaut, ou commit=true explicite)
curl -X POST \
  --data-binary @chunk_final.bin \
  "http://192.168.4.1/api/sequencer/upload-chunked"

# Response
{
  "status": "success",
  "message": "Séquence uploadée",
  "storage": {...}
}
```

#### Query Parameters
| Param | Type | Défaut | Description |
|-------|------|--------|-------------|
| `begin` | bool | false | Initialiser un nouvel upload |
| `commit` | bool | true | Finaliser l'upload |

---

### 3. GET /api/sequencer/download
**Télécharger le fichier séquenceur**

#### Request
```bash
curl -X GET \
  http://192.168.4.1/api/sequencer/download \
  -o nidmid.bin
```

#### Response (Success - 200)
```
Content-Type: application/octet-stream
Content-Disposition: attachment; filename="nidmid.bin"
Content-Length: 1024

[données binaires]
```

#### Response (Error - 404)
```json
{
  "status": "error",
  "code": "ERR_NOT_FOUND",
  "message": "Aucune séquence"
}
```

---

### 4. GET /api/sequencer/metadata
**Obtenir les métadonnées du fichier**

#### Request
```bash
curl -X GET http://192.168.4.1/api/sequencer/metadata | jq .
```

#### Response (File exists - 200)
```json
{
  "exists": true,
  "size": 1024,
  "valid": true,
  "crc32": "a1b2c3d4",
  "max_size": 524288,
  "storage": {
    "total": 524288,
    "used": 2048,
    "free": 522240
  }
}
```

#### Response (File missing - 200)
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

### 5. POST /api/sequencer/validate
**Valider l'intégrité du fichier (CRC32)**

#### Request
```bash
curl -X POST http://192.168.4.1/api/sequencer/validate | jq .
```

#### Response (Valid - 200)
```json
{
  "status": "valid",
  "exists": true,
  "read_status": 0,
  "header_valid": true,
  "checksum_valid": true,
  "data_size": 1024,
  "stored_crc32": "a1b2c3d4",
  "calculated_crc32": "a1b2c3d4"
}
```

#### Response (Invalid - 422)
```json
{
  "status": "invalid",
  "exists": true,
  "read_status": 0,
  "header_valid": true,
  "checksum_valid": false,
  "data_size": 1024,
  "stored_crc32": "a1b2c3d4",
  "calculated_crc32": "efgh5678"
}
```

---

### 6. DELETE /api/sequencer/delete
**Supprimer la séquence**

#### Request
```bash
curl -X DELETE http://192.168.4.1/api/sequencer/delete
```

#### Response (Success - 200)
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

#### Response (Not found - 404)
```json
{
  "status": "error",
  "code": "ERR_NOT_FOUND",
  "message": "Aucune séquence"
}
```

---

### 7. POST /api/sequencer/restore-backup
**Restaurer depuis la copie de secours (.bak)**

#### Request
```bash
curl -X POST http://192.168.4.1/api/sequencer/restore-backup
```

#### Response (Success - 200)
```json
{
  "status": "success",
  "message": "Restoration réussie",
  "storage": {
    "total": 524288,
    "used": 1024,
    "free": 523264
  }
}
```

#### Response (No backup - 404)
```json
{
  "status": "error",
  "code": "ERR_NOT_FOUND",
  "message": "Aucune copie de secours"
}
```

---

### 8. POST /api/sequencer/reset
**Réinitialiser le magasin (supprimer tout)**

#### Request
```bash
curl -X POST http://192.168.4.1/api/sequencer/reset
```

#### Response (Success - 200)
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

## Codes d'erreur HTTP

| Code HTTP | Description | Causes possibles |
|-----------|-------------|-----------------|
| 200 | OK | Opération réussie |
| 400 | Bad Request | Données invalides, fichier vide |
| 403 | Forbidden | Authentification échouée |
| 404 | Not Found | Fichier non trouvé, backup manquant |
| 408 | Request Timeout | Upload interrompu, timeout |
| 413 | Payload Too Large | Fichier dépasse 512 KB |
| 422 | Unprocessable Entity | CRC32 invalide, données corrompues |
| 500 | Internal Server Error | Erreur d'écriture, I/O failure |
| 503 | Service Unavailable | LittleFS non monté |
| 507 | Insufficient Storage | Espace disque insuffisant |

---

## Exemples d'intégration

### Python
```python
import requests

# Upload simple
with open('nidmid.bin', 'rb') as f:
    response = requests.post(
        'http://192.168.4.1/api/sequencer/upload',
        data=f.read()
    )
    print(response.json())

# Metadata
response = requests.get('http://192.168.4.1/api/sequencer/metadata')
metadata = response.json()
print(f"Sequence size: {metadata.get('size')} bytes")

# Download
response = requests.get('http://192.168.4.1/api/sequencer/download')
with open('downloaded.bin', 'wb') as f:
    f.write(response.content)
```

### JavaScript (Browser)
```javascript
// Upload simple
async function uploadSequence(file) {
    const response = await fetch('http://192.168.4.1/api/sequencer/upload', {
        method: 'POST',
        body: file,
        headers: {
            'Content-Type': 'application/octet-stream'
        }
    });
    return await response.json();
}

// Metadata
async function getMetadata() {
    const response = await fetch('http://192.168.4.1/api/sequencer/metadata');
    return await response.json();
}

// Download
async function downloadSequence() {
    const response = await fetch('http://192.168.4.1/api/sequencer/download');
    const blob = await response.blob();
    const url = window.URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'nidmid.bin';
    a.click();
}
```

### Bash Script (Upload progressif)
```bash
#!/bin/bash

DEVICE="192.168.4.1"
FILE="nidmid.bin"
CHUNK_SIZE=4096

# Initialiser
curl -X POST "http://$DEVICE/api/sequencer/upload-chunked?begin=true"

# Envoyer chunks
SIZE=$(stat -f%z "$FILE" 2>/dev/null || stat -c%s "$FILE")
CHUNKS=$((($SIZE + $CHUNK_SIZE - 1) / $CHUNK_SIZE))

for i in $(seq 0 $((CHUNKS-1))); do
    OFFSET=$((i * CHUNK_SIZE))
    COMMIT="false"
    if [ $i -eq $((CHUNKS-1)) ]; then
        COMMIT="true"
    fi
    
    tail -c +$((OFFSET+1)) "$FILE" | head -c $CHUNK_SIZE | \
    curl -X POST \
        --data-binary @- \
        "http://$DEVICE/api/sequencer/upload-chunked?commit=$COMMIT"
    
    echo "Chunk $((i+1))/$CHUNKS envoyé"
done
```

---

## Gestion de la mémoire

### Upload simple
- ✅ Optimal pour petit/moyen fichiers (< 100 KB)
- Charge entièrement en RAM avant écriture
- Latence: ~500-2000 ms

### Upload chunked
- ✅ Optimal pour gros fichiers (> 100 KB)
- Buffer configurable (actuellement 512 KB max)
- Chunks de 4 KB
- Latence: ~100-200 ms par chunk

### Download
- ✅ Streaming (très efficace en mémoire)
- Pas de chargement complet en RAM
- Latence: ~2-5 s pour 512 KB

---

## Débogage

### Serial Debug
Le firmware affiche le statut sur Serial:
```
[SequencerAPI] POST /upload: 1024 bytes
[SequencerAPI] POST /upload-chunked (begin header)
[SequencerAPI] Chunk reçu: 4096 bytes
[SequencerFileStore] ✅ Séquence écrite: 1024 bytes
```

### Status du magasin
```cpp
// Depuis le code C++:
SequencerFileStore::getInstance().debugPrintStatus();

// Output:
// [SequencerFileStore] === STATUS ===
// Ready: true
// Sequence exists: true
// Sequence size: 1024 bytes
// Sequence valid: true
// Storage: 2048 / 524288 bytes (free: 522240)
// ==============
```

### Validation CRC32
```bash
# Valider depuis le shell:
curl -X POST http://192.168.4.1/api/sequencer/validate | jq .

# Check CRC32:
# stored_crc32 == calculated_crc32 → valide ✅
# stored_crc32 != calculated_crc32 → corrompu ❌
```

---

## Test complet (workflow)

```bash
#!/bin/bash

DEVICE="192.168.4.1"

echo "1. Métadonnées avant"
curl -s http://$DEVICE/api/sequencer/metadata | jq .

echo "2. Upload du fichier"
curl -s -X POST --data-binary @test.bin http://$DEVICE/api/sequencer/upload | jq .

echo "3. Métadonnées après"
curl -s http://$DEVICE/api/sequencer/metadata | jq .

echo "4. Valider"
curl -s -X POST http://$DEVICE/api/sequencer/validate | jq .

echo "5. Télécharger"
curl -s http://$DEVICE/api/sequencer/download > downloaded.bin

echo "6. Comparer"
diff test.bin downloaded.bin && echo "✅ Identique!" || echo "❌ Différent!"

echo "7. Réinitialiser"
curl -s -X POST http://$DEVICE/api/sequencer/reset | jq .
```

---

## Notes importantes

1. **CRC32 automatique**: Calculé et stocké dans l'en-tête (format NPMS)
2. **Backup automatique**: Avant chaque écriture (`/seq/nidmid.bak`)
3. **Recovery automatique**: Si le fichier principal est perdu au boot
4. **Atomic writes**: Temp file → rename (pas de corruption en cas de coupure)
5. **Limite de taille**: 512 KB aligné avec partition LittleFS
6. **Sans authentification**: Réseau local uniquement pour l'instant

---

## Améliorations futures

- [ ] Token d'authentification (Bearer token)
- [ ] Rate limiting par IP
- [ ] Webhook de notification après upload
- [ ] Compression gzip optionnelle
- [ ] Versioning du format (multiple versions supportées)
- [ ] Signature numérique des fichiers
