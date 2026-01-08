# 📊 État Actuel du Projet ESP32Server

## 🎯 Résumé de la Session

### 📅 Contexte Récent
- **Modularisation UI** : Terminée avec succès
- **Compression gzip** : Implémentée avec route `/bundle`
- **Bug encodage** : Résolu avec HTML minimal + streaming par chunks
- **Optimisation mémoire** : 58 KB → 26 KB (réduction 55%)
- **Localisation** : Système multi-langue (français/anglais) avec JSON

### ✅ Réalisations Récentes

#### Modularisation Interface Web
- ✅ Code JavaScript organisé en 7 modules logiques
- ✅ HTML minimal séparé du JavaScript
- ✅ Structure maintenable et évolutive

#### Compression et Optimisation
- ✅ Bundle JavaScript compressé en gzip (75% de réduction)
- ✅ HTML minimal (~15.6 KB) avec `<script src="/bundle"></script>`
- ✅ Route `/bundle` avec en-tête `Content-Encoding: gzip`
- ✅ Streaming par chunks depuis PROGMEM pour fiabilité
- ✅ Minification JavaScript (suppression commentaires `/* */`)

#### Localisation Multi-langue
- ✅ Système de traduction basé sur JSON
- ✅ Support français (défaut) et anglais
- ✅ Option `--lang` dans `esp32server.sh`
- ✅ Placeholders `{{t.xxx}}` dans HTML/JS
- ✅ Documentation complète dans `docs/GUIDE_LOCALISATION.md`

#### Résolution Bugs
- ✅ Bug encodage aléatoire résolu (HTML minimal + streaming)
- ✅ Délai WebSocket pour éviter concurrence HTTP/WebSocket
- ✅ Intervalle `loadStatus` augmenté pour réduire charge

## 🏗️ Architecture Actuelle

### 📁 Structure du Projet
```
NiDMI/
├── src/
│   ├── ui_index.cpp       # HTML minimal (PROGMEM)
│   ├── ui_index.h         # Déclaration HTML
│   ├── ui_bundle.h        # Bundle JS gzipé (PROGMEM)
│   ├── WebAPI.cpp         # Routes HTTP (/ et /bundle)
│   ├── ComponentManager.cpp
│   ├── PinMapper.cpp
│   └── ...
├── web/
│   ├── index.html         # HTML minimal (source de vérité)
│   ├── app.js            # Code JS principal résiduel
│   ├── lang/
│   │   ├── fr.json       # Traductions françaises
│   │   └── en.json       # Traductions anglaises
│   └── js/
│       ├── core.js        # Utilitaires de base
│       ├── api.js         # Appels API
│       ├── pins.js        # Gestion des pins
│       ├── components.js  # Fonctions génériques
│       ├── websocket.js   # WebSocket temps réel
│       └── mux.js         # Multiplexeurs analogiques
├── scripts/
│   ├── build_html_simple.sh  # Build HTML + bundle gzip + traductions
│   └── esp32server.sh        # Script principal (intègre build)
├── build/                  # Fichiers générés (ignorés par git)
│   ├── index.min.html     # HTML minifié
│   └── bundle.js.gz       # Bundle JS compressé
└── docs/
    ├── GUIDE_LOCALISATION.md  # Guide localisation multi-langue
    ├── MODULARISATION_UI.md   # Guide modularisation
    ├── GUIDE_IMPLÉMENTATION_COMPOSANTS.md  # Guide pour stagiaires
    └── ...
```

### 🔧 Bibliothèques Utilisées
- **ESPAsyncWebServer** : Serveur HTTP + WebSocket
- **AsyncTCP** : TCP asynchrone pour ESP32
- **ESP32** : Framework 3.3.0+
- **Preferences** : Stockage NVS configuration
- **ESPmDNS** : Découverte de services

## 🎵 Fonctionnalités

### ✅ Fonctionnalités Opérationnelles
- **RTP-MIDI** : Configuration et transmission complète
- **OSC** : Support complet (Float 0-1 et MIDI 3 int)
- **WebSocket** : Synchronisation temps réel des configurations
- **Composants** : Boutons, potentiomètres, LEDs, Multiplexeurs
- **Interface Web** : Modulaire, optimisée, responsive, multi-langue
- **Gestion Pins** : Conflits automatiques, grisage des bus

### 🔄 Synchronisation WebSocket
- Configuration pins en temps réel
- Valeurs par défaut intelligentes (A0→CC#1, D0→Note 60, etc.)
- Gestion automatique des conflits (A0↔D0, SDA↔D4, etc.)
- Grisage automatique des pins de bus (I2C/SPI/UART)

## 📊 Métriques de Performance

### Mémoire Flash ESP32
- **Programme total** : 1198554 bytes (91% de 1310720 bytes)
- **Marge disponible** : 112166 bytes (9%)
- **Statut** : ✅ Marge suffisante pour évolutions futures

### Taille Interface Web
- **HTML minimal** : ~15.6 KB (structure + CSS)
- **Bundle JS (gzip)** : ~10.4 KB (43 KB source → 75% réduction)
- **Total** : ~26 KB (au lieu de 58 KB intégré)
- **Réduction** : 55% par rapport à version monolithique

## 🛠️ Workflow de Développement

### Modification de l'Interface Web

1. **Éditer les fichiers sources** :
   ```bash
   # Modifier HTML/CSS
   vim web/index.html
   
   # Modifier JavaScript (module spécifique)
   vim web/js/pins.js          # Gestion pins
   vim web/js/components.js    # Composants génériques
   vim web/js/api.js           # Appels API
   vim web/js/websocket.js     # WebSocket
   vim web/js/mux.js           # Multiplexeurs
   ```

2. **Build automatique** :
   ```bash
   # Build + Sync + Compile + Upload (français, défaut)
   ./scripts/esp32server.sh upload
   
   # Build + Sync + Compile + Upload (anglais)
   ./scripts/esp32server.sh upload --lang en
   
   # Ou seulement build + sync
   ./scripts/esp32server.sh sync
   ```

3. **Vérification** :
   - Ouvrir http://192.168.4.1 (ou nom.local)
   - Console navigateur (F12) pour debug
   - Vérifier logs ESP32 via moniteur série

### Scripts Disponibles

- `esp32server.sh sync` : Build HTML + synchronisation fichiers
- `esp32server.sh compile` : Build + compilation
- `esp32server.sh upload` : Build + compile + upload ESP32
- `esp32server.sh upload --lang en` : Build en anglais + upload
- `esp32server.sh monitor` : Moniteur série
- `build_html_simple.sh` : Build manuel HTML + bundle (si besoin)

## 🔍 Points Techniques Importants

### Build Process
1. **Chargement traductions** : Lecture du fichier JSON selon `LANG_CODE` (fr/en)
2. **Remplacement placeholders** : `{{t.xxx}}` remplacés dans HTML et JS
3. **Concaténation JS** : Modules concaténés dans l'ordre (`core.js` → `api.js` → `pins.js` → `components.js` → `websocket.js` → `mux.js` → `app.js`)
4. **Minification JS** : Suppression commentaires `/* */` (y compris multi-lignes)
5. **Génération HTML minimal** : Remplace `<!--JS-->...<!--/JS-->` par `<script src="/bundle"></script>`
6. **Compression gzip** : Bundle JS compressé avec `gzip -c`
7. **Conversion C++** : 
   - HTML → `src/ui_index.cpp` (format `R"rawliteral(...)rawliteral"`)
   - Bundle → `src/ui_bundle.h` (format PROGMEM avec `xxd -i`)
8. **Minification HTML** : Suppression commentaires HTML/JS et espaces multiples

### Routes HTTP
- **`/`** : HTML minimal (streaming par chunks depuis PROGMEM)
- **`/bundle`** : Bundle JS gzipé avec en-tête `Content-Encoding: gzip`
- **`/api/*`** : Endpoints API REST
- **`/ws`** : WebSocket pour synchronisation temps réel

### Format des Fichiers Générés

**`src/ui_index.cpp`** :
```cpp
#include "ui_index.h"
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
...
<script src="/bundle"></script>
...
)rawliteral";
```

**`src/ui_bundle.h`** :
```cpp
#ifndef UI_BUNDLE_H
#define UI_BUNDLE_H

#define BUNDLE_LEN 10439
const uint8_t BUNDLE[] PROGMEM = {
  0x1f, 0x8b, 0x08, ...
};
#endif
```

### Localisation

**Fichiers JSON** : `web/lang/fr.json`, `web/lang/en.json`

**Prérequis** : `jq` installé (`brew install jq` sur macOS)

**Placeholders** : Utiliser `{{t.section.key}}` dans HTML et JS

**Documentation** : `docs/GUIDE_LOCALISATION.md`

## ✅ État de Développement

### Fonctionnalités Stables
- ✅ Interface web modulaire et optimisée
- ✅ Compression gzip pour réduction taille
- ✅ WebSocket temps réel fonctionnel
- ✅ Gestion automatique conflits pins
- ✅ Multiplexeurs analogiques (HC4067)
- ✅ RTP-MIDI complet
- ✅ OSC complet (Float et MIDI)
- ✅ Sauvegarde NVS automatique
- ✅ Localisation multi-langue (fr/en)

### Optimisations Réalisées
- ✅ Modularisation JavaScript (7 modules)
- ✅ Compression gzip (75% réduction JS)
- ✅ HTML minimal séparé
- ✅ Streaming par chunks (résout bug encodage)
- ✅ Délai WebSocket pour éviter concurrence
- ✅ Minification automatique (commentaires `/* */`)
- ✅ Système de traduction JSON

## 🎯 Prochaines Évolutions Possibles

### Court Terme
- [ ] Implémenter les placeholders `{{t.xxx}}` dans `web/index.html`
- [ ] Ajouter traductions dans les fichiers JavaScript
- [ ] Tests unitaires pour modules JS
- [ ] Documentation API WebSocket

### Moyen Terme
- [ ] Support USB-MIDI
- [ ] Interface ESP32-S3 optimisée
- [ ] Touch pins ESP32-S3
- [ ] Debug avancé avec logs structurés
- [ ] Ajouter d'autres langues (espagnol, allemand, etc.)

### Long Terme
- [ ] Lazy loading modules JS (si nécessaire)
- [ ] Service Worker pour cache offline
- [ ] Compression CSS séparée (si taille augmente)
- [ ] Support multi-cartes

## 📋 Notes de Développement

### Ordre de Chargement des Modules
Les modules JavaScript doivent être chargés dans l'ordre :
1. `core.js` (dépendances de base : `$`, variables globales)
2. `api.js` (dépend de `core.js`)
3. `pins.js` (dépend de `core.js`)
4. `components.js` (dépend de `core.js`)
5. `websocket.js` (dépend de `core.js`)
6. `mux.js` (dépend de `components.js`)
7. `app.js` (code principal résiduel)

### Variables Globales Partagées
- `pcfg` : Configuration des pins
- `caps` : Capacités de la carte
- `cur` : Pin sélectionné actuellement
- `muxList` : Liste des multiplexeurs configurés
- `websocket` : Instance WebSocket

### Commentaires dans le Code
- ✅ **Autorisés** : Commentaires `/* */` (supprimés par minification)
- ❌ **Interdits** : Commentaires `//` (sauf dans URLs)
- **Raison** : Simplifie la minification et évite problèmes d'encodage

### Localisation
- **Fichiers JSON** : `web/lang/fr.json`, `web/lang/en.json`
- **Prérequis** : `jq` installé
- **Placeholders** : Format `{{t.section.key}}` (ex: `{{t.tabs.status}}`)
- **Option** : `--lang LANG` dans `esp32server.sh` (défaut: `fr`)
- **Documentation** : `docs/GUIDE_LOCALISATION.md`

---

*Document mis à jour le : Janvier 2025*
*État du projet : Stable et optimisé*
*Dernières modifications : Localisation multi-langue ajoutée*
