# 📚 Exemples ESP32Server

Ce dossier contient tous les exemples pour la bibliothèque ESP32Server.

## 📋 Organisation

### 🚀 Sketches principaux

**`esp32server_basic_c3/`** - Sketch pour ESP32-C3 (XIAO_ESP32C3)
- Sketch minimal et stable pour ESP32-C3
- Toutes les fonctionnalités disponibles
- Pas de debug activé par défaut (silencieux)
- Pins disponibles : A0-A2, D0-D10 (A3 n'existe pas sur C3)
- ⚠️ **Sélectionner le board XIAO_ESP32C3 dans Arduino IDE**

**`esp32server_basic_s3/`** - Sketch pour ESP32-S3 (XIAO_ESP32S3)
- Sketch minimal et stable pour ESP32-S3
- Toutes les fonctionnalités disponibles
- Pas de debug activé par défaut (silencieux)
- Pins disponibles : A0-A4, D0-D9 (touch pins supportées)
- Touch pins : D0-D9 (toutes analogiques)
- USB MIDI : À venir
- ⚠️ **Sélectionner le board XIAO_ESP32S3 dans Arduino IDE**

**`esp32server_basic/`** - Sketch générique (legacy)
- Ancien sketch avec détection automatique
- Peut être utilisé mais les sketches spécifiques sont recommandés
- ⚠️ **Utiliser les sketches spécifiques (C3 ou S3) de préférence**

### Autres exemples (optionnels)
- **`components_basic/`** - Utilisation du ComponentManager

### 🎵 Exemples MIDI
- **`rtpmidi/`** - Exemples RTP-MIDI
  - `rtpmidi_basic/` - Configuration RTP-MIDI de base
  - `rtpmidi_advanced/` - Configuration avancée

### 🌐 Exemples OSC
- **`esp32server_osc/`** - Serveur OSC complet avec Pure Data

### 🐛 Debug
- **`_debug/`** - Sketches de debug pour diagnostiquer des problèmes
  - Voir `_debug/README.md` pour plus de détails

## 🎯 Usage rapide

### Dans Arduino IDE

1. **Pour ESP32-C3** :
   - Ouvrir `examples/esp32server_basic_c3/esp32server_basic_c3.ino`
   - Sélectionner le board : **XIAO_ESP32C3**
   - Uploader

2. **Pour ESP32-S3** :
   - Ouvrir `examples/esp32server_basic_s3/esp32server_basic_s3.ino`
   - Sélectionner le board : **XIAO_ESP32S3**
   - Uploader

### Avec le script

```bash
# Uploader le sketch C3
./scripts/esp32server.sh upload esp32server_basic_c3

# Uploader le sketch S3
./scripts/esp32server.sh upload esp32server_basic_s3

# Uploader avec un sketch optionnel
./scripts/esp32server.sh upload esp32server_osc
```

## 🔧 Différences entre C3 et S3

| Fonctionnalité | ESP32-C3 | ESP32-S3 |
|---------------|----------|----------|
| Pins analogiques | A0, A1, A2 | A0, A1, A2, A3, A4 |
| Pins digitales | D0-D10 | D0-D9 |
| Touch pins | ❌ Non | ✅ Oui (D0-D9) |
| USB MIDI | ❌ Non | 🔜 À venir |
| Détection MCU | Automatique (fallback GPIO) | Automatique (fallback GPIO) |

## 📝 Créer un nouvel exemple

1. Créer un dossier dans la catégorie appropriée
2. Créer un fichier `.ino` avec le même nom
3. Tester avec `./scripts/esp32server.sh upload <nom>`
4. Mettre à jour ce README

