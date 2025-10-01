# 📚 Exemples ESP32Server

Ce dossier contient tous les exemples pour la bibliothèque ESP32Server.

## 📋 Organisation

### 🚀 Exemples de base
- **`esp32server_basic/`** - Exemple minimal (recommandé pour débuter)
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

```bash
# Uploader un exemple
./scripts/esp32server.sh upload esp32server_basic

# Uploader avec un sketch personnalisé
./scripts/esp32server.sh upload esp32server_osc

# Debug
./scripts/esp32server.sh upload _debug/esp32server_debug
```

## 📝 Créer un nouvel exemple

1. Créer un dossier dans la catégorie appropriée
2. Créer un fichier `.ino` avec le même nom
3. Tester avec `./scripts/esp32server.sh upload <nom>`
4. Mettre à jour ce README

