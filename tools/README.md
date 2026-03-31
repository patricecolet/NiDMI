# Outils NiDMI

## Partition ESP32-C3 sans SPIFFS (`nidmi_c3_no_spiffs.csv`)

Table de partitions pour **ESP32-C3 (4 Mo flash)** sans partition SPIFFS, avec une partition application agrandie (~4 Mo au lieu de ~1,25 Mo).

**Utilité :** évite d’atteindre 97 % du flash (firmware trop serré). Le projet n’utilise pas SPIFFS (HTML/JS en PROGMEM), donc supprimer SPIFFS est sans impact fonctionnel.

**Comportement par défaut :** pour le C3, le script active cette partition automatiquement. Pour désactiver : `--no-large-app`.

```bash
./scripts/nidmi.sh compile --board c3    # partition 4 Mo activée par défaut
./scripts/nidmi.sh compile --board c3 --no-large-app   # revenir à la partition standard
```

Le script copie ce fichier dans le package Arduino ESP32 (`tools/partitions/`) puis compile avec `build.partitions=nidmi_c3_no_spiffs` et `upload.maximum_size=4161536`.
