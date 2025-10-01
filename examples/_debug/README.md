# 🐛 Sketches de Debug

Ce dossier contient des sketches de debug spécifiques pour diagnostiquer et résoudre des problèmes.

## 📋 Organisation

- **`esp32server_debug/`** - Debug du grisage automatique des pins I2C/SPI
  - Active : NETWORK, WEBSOCKET, API, CACHE
  - Problème : Les pins I2C/SPI ne se grisent pas automatiquement

## 🎯 Usage

```bash
# Uploader un sketch de debug
./scripts/esp32server.sh upload _debug/esp32server_debug

# Moniteur série
./scripts/esp32server.sh monitor
```

## 🔧 Ajouter un nouveau sketch de debug

1. Créer un dossier dans `_debug/`
2. Créer un fichier `.ino` avec les macros de debug activées
3. Ajouter une description dans ce README
4. Uploader et tester

## 📝 Notes

- Tous les sketches de debug ont le Serial activé
- Les macros de debug doivent être définies **AVANT** les includes
- Voir `esp32server_debug.h` pour la liste complète des macros

