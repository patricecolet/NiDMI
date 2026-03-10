# 📘 Guide du Script `nidmi.sh`

Script unifié pour le développement NiDMI : synchronisation, compilation, upload et génération de binaires.

## 🎯 Vue d'ensemble

Le script `nidmi.sh` automatise les tâches de développement pour NiDMI :
- Synchronisation des fichiers source vers la bibliothèque Arduino
- Minification de l'interface web
- Compilation des sketches
- Upload du firmware
- Monitor série

## 📋 Prérequis

- **arduino-cli** : Installé et configuré
  - macOS : `brew install arduino-cli`
  - Linux : Voir [arduino-cli documentation](https://arduino.github.io/arduino-cli)
- **jq** : Pour la localisation (optionnel)
  - macOS : `brew install jq`
- Bibliothèque NiDMI installée dans Arduino IDE
- Core ESP32 installé dans Arduino CLI

## 🚀 Utilisation de base

### Syntaxe générale

```bash
./scripts/nidmi.sh [OPTION] [SKETCH] [--lang LANG] [--board BOARD]
```

### Commandes principales

| Commande | Description |
|----------|-------------|
| `sync` | Synchroniser les fichiers seulement (pas de compilation) |
| `compile` | Synchroniser + compiler |
| `build` | Synchroniser + compiler + stocker binaire + générer UF2 |
| `upload` | Synchroniser + compiler + uploader via USB série |
| `monitor` | Ouvrir le moniteur série |
| `all` | Tout faire (sync + compile + upload + test) |
| `clean` | Nettoyer le cache Arduino seulement |
| `help` | Afficher l'aide |

## ⚙️ Options

### Option `--lang`

Définit la langue de l'interface web.

**Valeurs possibles :**
- `fr` (défaut) : Français
- `en` : Anglais

**Exemple :**
```bash
./scripts/nidmi.sh sync --lang en
./scripts/nidmi.sh upload --lang en
```

**Note :** Nécessite `jq` installé pour les traductions.

### Option `--board`

Spécifie le type de carte ESP32.

**Valeurs possibles :**
- `c3` (défaut) : XIAO ESP32-C3
- `s3` : XIAO ESP32-S3

**Exemple :**
```bash
./scripts/nidmi.sh compile --board s3
./scripts/nidmi.sh upload --board s3
```

**FQBN correspondants :**
- `c3` → `esp32:esp32:XIAO_ESP32C3`
- `s3` → `esp32:esp32:XIAO_ESP32S3`

### Pagination (par défaut, C3 et S3)
   - La pagination est activée par défaut pour C3 et S3.
   - L'API renvoie les définitions de composants par pages (12 Ko).

**Options :**
- `--pagination`
- `--no-pagination`

### Partition C3 (large-app, par défaut pour C3 uniquement)
Sur C3, le script utilise par défaut une partition sans SPIFFS, 4 Mo app `tools/nidmi_c3_no_spiffs.csv`

**Comment la désactiver?** : `--no-large-app`

Et, ça ne s’applique pas au S3.


### Résumé des défauts :
- La pagination est toujours activée pour C3 et S3.
- La partition large-app est activée par défaut sur C3 et elle est désactivable.

## 📖 Exemples d'utilisation

### Synchronisation simple

```bash
# Synchroniser les fichiers (français, C3 par défaut)
./scripts/nidmi.sh sync

# Synchroniser en anglais
./scripts/nidmi.sh sync --lang en
```

### Compilation

```bash
# Compiler pour ESP32-C3 (défaut)
./scripts/nidmi.sh compile

# Compiler pour ESP32-S3
./scripts/nidmi.sh compile --board s3

# Compiler un sketch spécifique
./scripts/nidmi.sh compile nidmi_osc
```

### Build complet (binaire + UF2)

```bash
# Build pour ESP32-C3
./scripts/nidmi.sh build

# Build pour ESP32-S3
./scripts/nidmi.sh build --board s3
```

Les fichiers binaires sont stockés dans `bin/` :
- `nidmi_basic.ino.bin` : Binaire principal
- `nidmi_basic.ino.merged.bin` : Binaire fusionné
- `nidmi_basic.uf2` : Format UF2 (généré automatiquement)

### Upload via USB série

```bash
# Upload sur ESP32-C3 (défaut)
./scripts/nidmi.sh upload

# Upload sur ESP32-S3
./scripts/nidmi.sh upload --board s3
```

**Note :** Le script détecte automatiquement le port série USB (`/dev/cu.usbmodem*`, `/dev/cu.usbserial-*`, etc.).

### Monitor série

```bash
# Ouvrir le monitor série
./scripts/nidmi.sh monitor
```

**Caractéristiques :**
- Détection automatique du port série
- Vitesse : 115200 bauds
- Pour quitter : `Ctrl+C`

### Workflow complet

```bash
# Tout faire : sync + compile + upload + test
./scripts/nidmi.sh all

# Avec options
./scripts/nidmi.sh all --board s3 --lang en
```

Le workflow `all` :
1. Synchronise les fichiers
2. Nettoie le cache
3. Compile le sketch
4. Upload le firmware
5. Attend 5 secondes
6. Ouvre Firefox sur `http://192.168.4.1`

## 🔧 Détails techniques

### Synchronisation (`sync`)

La synchronisation effectue :

1. **Minification de l'UI** :
   - Appelle `build_html_simple.sh`
   - Génère `src/ui_index.cpp` (HTML minifié)
   - Génère `src/ui_bundle.h` (JS gzipé)

2. **Copie des fichiers** :
   - `src/*.cpp` et `src/*.h` → bibliothèque Arduino
   - `src/api/*` → bibliothèque Arduino
   - `src/components/*` → bibliothèque Arduino
   - `src/midi/*` → bibliothèque Arduino
   - `examples/*` → bibliothèque Arduino

3. **Nettoyage du cache** :
   - Nettoie le cache Arduino (`/Library/Caches/arduino/sketches`)

### Compilation (`compile`)

- Utilise `arduino-cli compile`
- FQBN selon `--board` (C3 ou S3)
- Sketch : `examples/nidmi_basic/nidmi_basic.ino` (par défaut)

### Build (`build`)

- Compile avec `--output-dir bin/`
- Génère automatiquement le fichier UF2
- Stocke tous les binaires dans `bin/`

### Génération UF2

La fonction `generate_uf2()` :
- Convertit `*.merged.bin` en format UF2
- Utilise `xxd` (pas de dépendance Python)
- Format compatible pour drag-and-drop (note : UF2 natif non supporté sur ESP32, utiliser `esptool.py`)

### Upload (`upload`)

- Détecte automatiquement le port série
- Utilise `arduino-cli upload`
- FQBN selon `--board`

### Monitor (`monitor`)

- Détecte automatiquement le port série
- Utilise `arduino-cli monitor`
- Vitesse : 115200 bauds
- Configuration robuste

## 📁 Structure des fichiers

### Chemins par défaut

- **Répertoire source** : `/Users/patricecolet/repo/NiDMI`
- **Bibliothèque Arduino** : `/Users/patricecolet/Documents/Arduino/libraries/NiDMI`
- **Cache Arduino** : `/Users/patricecolet/Library/Caches/arduino/sketches`
- **Binaires** : `/Users/patricecolet/repo/NiDMI/bin`

**Note :** Ces chemins sont codés en dur dans le script (ligne 17-19).

### Fichiers générés

- `build/index.min.html` : HTML minifié
- `build/bundle.js.gz` : JS compressé
- `src/ui_index.cpp` : HTML en C++ (PROGMEM)
- `src/ui_bundle.h` : JS gzipé en C++ (PROGMEM)
- `bin/*.bin` : Binaires compilés
- `bin/*.uf2` : Fichiers UF2

## ⚠️ Dépannage

### Erreur : "arduino-cli non trouvé"

**Solution :**
```bash
# macOS
brew install arduino-cli

# Linux
# Voir : https://arduino.github.io/arduino-cli
```

### Erreur : "jq non trouvé"

**Solution :**
```bash
# macOS
brew install jq

# Linux
sudo apt-get install jq
```

**Note :** `jq` n'est nécessaire que pour `--lang` (traductions). Si vous n'utilisez pas les traductions, cette erreur peut être ignorée.

### Erreur : "Aucun port série trouvé"

**Causes possibles :**
- ESP32 non connecté
- Câble USB défectueux
- Pilotes USB non installés

**Solutions :**
1. Vérifier la connexion USB
2. Vérifier les ports disponibles : `ls /dev/cu.*`
3. Installer les pilotes USB (si nécessaire)

### Erreur : "Resource busy" ou "Port is busy"

**Cause :** Le port série est occupé par un autre processus (monitor série, Arduino IDE, etc.).

**Solution :**
1. **Fermer le monitor série** : Dans Arduino IDE, fermer le moniteur série (Outils → Moniteur série)
2. **Tuer le processus** : Si le monitor série ne se ferme pas :
   ```bash
   # Trouver le processus qui utilise le port
   lsof | grep /dev/cu.usbmodem14301
   
   # Tuer le processus (remplacer PID par le numéro trouvé)
   kill PID
   ```
3. **Fermer Arduino IDE** : Si nécessaire, fermer complètement Arduino IDE et réessayer

### Erreur : "Port non accessible"

**Solution :**
```bash
# macOS : Ajouter l'utilisateur au groupe dialout
sudo dseditgroup -o edit -a $USER -t user dialout

# Ou utiliser sudo (non recommandé)
sudo ./scripts/nidmi.sh monitor
```

### Erreur de compilation : "multiple definition"

**Cause :** Fichiers obsolètes dans la bibliothèque Arduino.

**Solution :**
1. Supprimer manuellement les anciens fichiers dans `/Users/patricecolet/Documents/Arduino/libraries/NiDMI/src/`
2. Relancer la synchronisation

### Erreur : "Fichier merged.bin non trouvé"

**Cause :** La compilation n'a pas généré le fichier `merged.bin`.

**Solution :**
- Utiliser `build` au lieu de `compile`
- Vérifier que la compilation a réussi

## 🔄 Workflow recommandé

### Développement quotidien

```bash
# 1. Faire des modifications dans le code
# 2. Synchroniser et compiler
./scripts/nidmi.sh compile

# 3. Uploader
./scripts/nidmi.sh upload

# 4. Tester via le monitor
./scripts/nidmi.sh monitor
```

### Build pour distribution

```bash
# 1. Build complet (génère binaires + UF2)
./scripts/nidmi.sh build --board c3
./scripts/nidmi.sh build --board s3

# 2. Les binaires sont dans bin/
ls -lh bin/
```

### Debug

```bash
# 1. Ouvrir le monitor
./scripts/nidmi.sh monitor

# 2. Voir les logs en temps réel
# 3. Appuyer sur RESET pour voir les logs de démarrage
```

## 📝 Notes importantes

1. **Synchronisation manuelle requise** : Le script copie les fichiers mais ne supprime pas les anciens fichiers obsolètes. Si vous renommez des fichiers, supprimez-les manuellement de la bibliothèque Arduino.

2. **UF2 sur ESP32** : Le format UF2 est généré mais le drag-and-drop natif n'est pas supporté sur ESP32. Utilisez `esptool.py` ou Arduino IDE pour uploader (voir `UPLOAD_FIRMWARE.md`).

3. **Chemins codés en dur** : Les chemins dans le script sont spécifiques à macOS. Pour Linux, modifiez les variables en haut du script.

4. **Board par défaut** : Si `--board` n'est pas spécifié, le script utilise ESP32-C3 par défaut.

5. **Cache** : Le script nettoie automatiquement le cache Arduino pour éviter les problèmes de compilation.

## 🔗 Voir aussi

- `README.md` : Documentation générale du projet
- `UPLOAD_FIRMWARE.md` : Guide d'upload du firmware
- `ADVANCED.md` : Documentation avancée (MIDI, OSC, etc.)
