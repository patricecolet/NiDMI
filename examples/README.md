# 📚 Exemples ESP32Server

Ce dossier contient tous les exemples pour la bibliothèque ESP32Server.

## 📋 Organisation

### 🚀 Sketch principal

**`esp32server_basic/`** - Sketch UNIQUE et PRINCIPAL (recommandé pour tous les usages)
- Sketch minimal et stable
- Toutes les fonctionnalités disponibles
- Pas de debug activé par défaut (silencieux)
- Modifications uniquement si nécessaire pour avancer
- ⚠️ **Ce sketch remplace tous les autres sketches de debug**

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

```bash
# Uploader le sketch principal (recommandé)
./scripts/esp32server.sh upload esp32server_basic

# Ou simplement (esp32server_basic est le défaut)
./scripts/esp32server.sh upload

# Uploader avec un sketch optionnel
./scripts/esp32server.sh upload esp32server_osc
```

## 📝 Créer un nouvel exemple

1. Créer un dossier dans la catégorie appropriée
2. Créer un fichier `.ino` avec le même nom
3. Tester avec `./scripts/esp32server.sh upload <nom>`
4. Mettre à jour ce README

