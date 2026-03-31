# Outils NiDMI

## Partition ESP32-C3 sans SPIFFS (`nidmi_c3_no_spiffs.csv`)

Table de partitions pour **ESP32-C3 (4 Mo flash)** sans partition SPIFFS, avec une partition application agrandie (~4 Mo au lieu de ~1,25 Mo).

**Utilité :** évite d’atteindre 97 % du flash (firmware trop serré). Le projet n’utilise pas SPIFFS (HTML/JS en PROGMEM), donc supprimer SPIFFS est sans impact fonctionnel.

**Comportement par défaut :** pour le C3, le script active cette partition automatiquement. Pour désactiver : `--no-large-app`.

```bash
./scripts/nidmi.sh compile --board c3    # partition 4 Mo activée par défaut
./scripts/nidmi.sh compile --board c3 --no-large-app   # revenir à la partition standard
```

Le script copie ce fichier dans le package Arduino ESP32 (`tools/partitions/`) puis compile avec `build.partitions=nidmi_c3_no_spiffs` et `upload.maximum_size=4063232`.

## Partitions split-fs (2x LittleFS dédiés)

Deux profils supplémentaires sont disponibles pour séparer les données:

- `nidmi_c3_dual_littlefs.csv` (ESP32-C3 4MB)
  - `seqfs` = 128KB (fichiers de séquence)
  - `mapfs` = 128KB (scripts de mapping)
  - `app0` = 0x3A0000 (3 801 088 bytes)
- `nidmi_s3_dual_littlefs.csv` (ESP32-S3 8MB)
  - `seqfs` = 512KB
  - `mapfs` = 1MB
  - `app0` = 0x660000 (6 684 672 bytes)

Activation via script:

```bash
./scripts/nidmi.sh compile --board c3 --split-fs
./scripts/nidmi.sh compile --board s3 --split-fs
```
