# NiDMI (librairie Arduino)

Serveur web simple (HTTP + WebSocket) pour ESP32‑C3/S3, destiné à des ateliers capteurs/actuateurs (musique électroacoustique et actuelle).

- Documentation avancée (MIDI / OSC / Temps réel): consultez `docs/ADVANCED.md`.
- **Serveur web sur ESP32‑C3** (mémoire, API définitions, JSON compact, pagination): `docs/SERVEUR_WEB_ESP32_C3.md`.

## Installation

### Installation automatique (Arduino IDE 2.x) — Recommandé

1. Ouvrir l'IDE Arduino 2.x
2. Aller dans **Croquis > Inclure une bibliothèque > Gérer les bibliothèques...**
3. Rechercher "NiDMI" et cliquer sur **Installer**
4. Arduino IDE installera automatiquement toutes les dépendances requises :
   - AsyncTCP
   - ESPAsyncWebServer
   - AppleMIDI
5. Installer le core ESP32 si ce n'est pas déjà fait : **Outils > Type de carte > Gestionnaire de cartes...** → chercher « esp32 » (Espressif Systems) → Installer
6. Ouvrir un exemple : **Fichier > Exemples > NiDMI > nidmi_basic**

**Note** : Si vous installez via un fichier ZIP (Croquis > Inclure une bibliothèque > Ajouter une bibliothèque .ZIP...), Arduino IDE vous proposera automatiquement d'installer les dépendances manquantes.

### Installation manuelle (Arduino IDE 1.x ou avancé)

Si vous utilisez Arduino IDE 1.x ou préférez installer manuellement :

1. Installer le core ESP32 : **Outils > Type de carte > Gestionnaire de cartes...** → chercher « esp32 » (Espressif Systems) → Installer
2. Installer les dépendances via le Gestionnaire de bibliothèques :
   - « ESP Async WebServer »
   - « AsyncTCP »
   - « AppleMIDI »
3. Installer cette bibliothèque via **Croquis > Inclure une bibliothèque > Ajouter une bibliothèque .ZIP...**
4. Ouvrir un exemple : **Fichier > Exemples > NiDMI > nidmi_basic**

### Installation avec arduino-cli (avancé)

Pour une installation en ligne de commande :

- macOS/Linux: `bash scripts/install_deps.sh`
- Windows: `PowerShell -ExecutionPolicy Bypass -File scripts/install_deps.ps1`

Ces scripts installent le core `esp32:esp32` et les librairies requises.

## Fonctionnalités

### 🔄 **Synchronisation WebSocket** (Nouveau !)
- **Configuration temps réel** des pins via WebSocket
- **Valeurs par défaut intelligentes** : A0→CC#1, A1→CC#2, D0→Note 60, D1→Note 61, etc.
- **Gestion automatique des conflits** : A0↔D0, SDA↔D4, MOSI↔D8
- **Grisage des pins de bus** : I2C/SPI bloquent automatiquement les pins associées
- **Configuration OSC/Debug** intégrée par défaut

### 🎵 **MIDI & OSC** (Amélioré !)
- **Support RTP-MIDI complet** avec configuration par pin
- **OSC avancé** : Format configurable (Float 0-1 ou MIDI 3 int)
- **Broadcast OSC** : Support AP/STA et IP spécifique
- **Interface web intuitive** avec synchronisation temps réel
- **Sauvegarde NVS automatique** de toutes les configurations

## Utilisation

### Exemple de base

```cpp
#include <NiDMI.h>

void setup() {
  Serial.begin(115200);
  
  // Démarrer le serveur avec nom personnalisé
  NiDMI.begin("MonServeur");
  
  // Attendre la connexion WiFi
  while (!NiDMI.isConnected()) {
    delay(100);
  }
  
  Serial.println("Serveur prêt !");
  Serial.print("IP: ");
  Serial.println(NiDMI.getIP());
}

void loop() {
  NiDMI.update();
}
```

### Configuration via interface web

1. **Connexion** : L'ESP32 crée un point d'accès WiFi `NiDMI-XXXX`
2. **Interface** : Ouvrir `http://192.168.4.1` dans un navigateur
3. **Configuration** :
   - **WiFi** : Nom du réseau et mot de passe
   - **MIDI** : Nom du périphérique RTP-MIDI
   - **OSC** : Format (Float/MIDI), broadcast, adresses personnalisées
   - **Pins** : Configuration des entrées/sorties avec OSC
4. **Sauvegarde** : Les paramètres sont stockés en mémoire

### Fonctionnalités principales

- **🌐 Serveur web** : Interface de configuration intuitive
- **📡 RTP-MIDI** : Connexion sans fil avec macOS/Logic
- **🎛️ OSC avancé** : Format configurable, broadcast, adresses personnalisées
- **🔌 Pins configurables** : Entrées analogiques, boutons, LEDs
- **📱 BLE MIDI** : Support Bluetooth Low Energy MIDI (optionnel)
- **👆 Touch pins** : Support des touch pins ESP32-S3 (en développement)
- **⚡ Temps réel** : Latence optimisée pour la musique
- **💾 Stockage** : Configuration persistante

## Architecture

### Composants principaux

- **`NiDMIServer`** : Classe principale, gestion WiFi et serveur web
- **`ComponentManager`** : Gestion des pins et composants
- **`PinMapper`** : Mapping des pins ESP32-C3/S3
- **`RtpMidi`** : Communication MIDI sans fil
- **`WebAPI`** : Interface REST pour la configuration

### Structure des fichiers

```
src/
├── NiDMIServer.cpp/h          # Classe principale
├── ComponentManager.cpp/h      # Gestion des composants
├── PinMapper.cpp/h            # Mapping des pins
├── RtpMidi.cpp/h             # RTP-MIDI
├── WebAPI.cpp                 # API REST
├── ServerCore.cpp/h           # Cœur du serveur
└── ui_index.cpp/h             # Interface web intégrée
```

## Configuration des pins

### ESP32-C3 (XIAO_ESP32C3)

```cpp
// Pins disponibles
const uint8_t ANALOG_PINS[] = {A0, A1, A2, A3};
const uint8_t DIGITAL_PINS[] = {2, 3, 4, 5, 6, 7, 8, 9, 10};
```

### ESP32-S3

```cpp
// Pins disponibles (à configurer selon votre carte)
const uint8_t ANALOG_PINS[] = {A0, A1, A2, A3, A4, A5};
const uint8_t DIGITAL_PINS[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

// Touch pins ESP32-S3 (fonctionnalité en développement)
const uint8_t TOUCH_PINS[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}; // GPIO1-10
```

### Types de composants supportés

- **Potentiomètres** : Entrées analogiques (ADC)
- **Boutons** : Entrées digitales avec anti-rebond
- **LEDs** : Sorties digitales (PWM)
- **Touch pins** : Capteurs tactiles ESP32-S3 (en développement)

## API REST

### Endpoints disponibles

- **`GET /api/status`** : État du système
- **`GET /api/pins`** : Configuration des pins
- **`POST /api/pins`** : Modifier la configuration
- **`GET /api/midi`** : Configuration MIDI
- **`POST /api/midi`** : Modifier la configuration MIDI

### Exemple d'utilisation

```javascript
// Récupérer la configuration
fetch('/api/pins')
  .then(response => response.json())
  .then(data => console.log(data));

// Modifier une pin
fetch('/api/pins', {
  method: 'POST',
  headers: {'Content-Type': 'application/json'},
  body: JSON.stringify({
    pin: 2,
    type: 'button',
    midiChannel: 1,
    midiNote: 60
  })
});
```

## BLE MIDI (Bluetooth Low Energy)

### Activation

Pour activer le support BLE MIDI, ajoutez cette ligne au début de votre sketch :

```cpp
#define NIDMI_ENABLE_BLE_MIDI
#include <nidmi.h>
```

### Utilisation

```cpp
void setup() {
    nidmi_setup();
    
    // Configuration normale
    nidmi_addButton(2, 1, 60, 1);
    nidmi_addPotentiometer(6, 1, 1, 1);
}

void loop() {
    nidmi_loop();
}
```

### Connexion

1. **Rechercher** "ESP32-MIDI" dans les paramètres Bluetooth de votre appareil
2. **Se connecter** (pas de code PIN requis)
3. **Utiliser** avec des apps de terminal Bluetooth ou des apps MIDI

### Communication BLE

Le BLE fonctionne comme une communication série bidirectionnelle :

```cpp
// Données envoyées automatiquement quand vous appuyez sur un bouton
// Format : [status, data1, data2] (ex: [0x90, 60, 127] pour Note On)

// Données reçues via BLE (affichées dans le Serial Monitor)
// Vous pouvez envoyer des commandes depuis votre ordinateur/phone
```

### Limitations

- **Taille** : BLE augmente la taille du binaire (~200KB)
- **Compatibilité** : Fonctionne avec tous les appareils Bluetooth
- **Latence** : Légèrement plus élevée que RTP-MIDI
- **Format** : Communication série simple, pas MIDI standard

## Développement

**Note importante** : Les fichiers générés (`src/ui_index.cpp`, `src/ui_bundle.h`) sont déjà inclus dans le dépôt. Les utilisateurs finaux n'ont **pas besoin** d'outils de développement comme `xxd` ou Xcode pour installer et utiliser la bibliothèque.

### Modification de l'interface web

Si vous modifiez les fichiers source de l'interface web dans `web/`, vous devrez reconstruire les fichiers générés :

```bash
# Reconstruire l'interface web (nécessite xxd)
./scripts/build_html_simple.sh

# Ou utiliser le script complet qui fait tout
./scripts/nidmi.sh sync
```

**Prérequis pour le développement** :
- `xxd` : fourni par Xcode Command Line Tools sur macOS (`xcode-select --install`), ou disponible via Homebrew (`brew install xxd`) ou via les outils système sur Linux
- `jq` : nécessaire pour les traductions (optionnel, `brew install jq` sur macOS)

### Compilation

```bash
# Installation des dépendances
./scripts/install_deps.sh

# Compilation
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32C3 examples/nidmi_basic/nidmi_basic.ino
```

### Tests

```bash
# Build et test de compilation
./scripts/nidmi.sh compile

# Build complet + upload
./scripts/nidmi.sh upload

# Reset NVS (sketch nidmi_clear_nvs) — reprendre les mêmes options que ton upload habituel (ex. --split-fs seulement si tu l’utilises)
./scripts/nidmi.sh upload --clear-nvs
# Alternative esptool si besoin
./scripts/nidmi.sh erase-nvs --port /dev/cu.usbmodem1101

# Moniteur série pour logs
./scripts/nidmi.sh monitor
```

## Bugs connus et limitations

### 🐛 Écho MIDI RTP-MIDI

**Problème** : L'ESP32 retransmet les messages MIDI reçus, créant des boucles potentielles.

**Impact** : 
- Boucles MIDI dans le DAW (ex: CC7 → LED → CC7 → LED...)
- Obligation de router sur un autre contrôleur pour éviter la boucle
- Workflow de contrôle dégradé

**Workaround** : 
- Router le contrôleur MIDI sur un autre numéro dans le DAW
- Utiliser des délais dans le routage DAW
- Éviter les connexions directes CC → LED

**Status** : Bug connu, investigation en cours. L'écho semble provenir de la bibliothèque AppleMIDI ou du protocole RTP-MIDI lui-même.

### 🔧 Limitations actuelles

- **OSC** : Implémenté avec limitations connues (perte de paquets sur boutons WiFi)
- **USB-MIDI** : Non implémenté (en développement)  
- **ESP32-S3** : Interface web à adapter
- **Touch pins** : Support ESP32-S3 en développement
- **Debug** : Logs limités (en développement)

## Roadmap

### 🚀 Fonctionnalités en développement

- **OSC** : Support Open Sound Control
- **USB-MIDI** : Connexion USB directe
- **ESP32-S3** : Support complet de l'ESP32-S3
- **Touch pins** : Support des touch pins ESP32-S3
- **Debug avancé** : Logs détaillés et monitoring
- **Interface améliorée** : Support multi-cartes

### 📋 Prochaines versions

- **v0.2.0** : Support OSC
- **v0.3.0** : USB-MIDI
- **v0.4.0** : ESP32-S3 complet + Touch pins
- **v0.5.0** : Debug et monitoring

## Optimisation de l'interface web

### Architecture Modulaire et Compression

L'interface web est **modularisée** et **compressée** pour optimiser l'utilisation de la mémoire flash :

- ✅ **7 modules JavaScript** organisés par responsabilité
- ✅ **HTML minimal** séparé du JavaScript (~15.6 KB)
- ✅ **Compression gzip** du bundle JS (75% de réduction)
- ✅ **Total optimisé** : 26 KB (au lieu de 58 KB) → **55% de réduction**

### Build Automatique

Le build est **automatique** via `nidmi.sh` :

```bash
# Build + Sync + Compile + Upload (recommandé)
./scripts/nidmi.sh upload

# Ou seulement build + sync
./scripts/nidmi.sh sync
```

**Le script `nidmi.sh`** :
1. Appelle automatiquement `build_html_simple.sh`
2. Génère `src/ui_index.cpp` (HTML minimal)
3. Génère `src/ui_bundle.h` (Bundle JS gzipé)
4. Synchronise les fichiers vers la librairie Arduino

### Structure Modulaire

```
web/
├── index.html          # HTML minimal avec <script src="/bundle"></script>
├── app.js             # Code JS principal résiduel
└── js/
    ├── core.js         # Utilitaires de base (variables globales, $)
    ├── api.js          # Appels API (loadStatus, loadMdns, etc.)
    ├── pins.js         # Gestion des pins (drawBoard, updatePinsList)
    ├── components.js   # Fonctions génériques pour composants
    ├── websocket.js    # WebSocket temps réel
    └── mux.js          # Multiplexeurs analogiques
```

### Développement de l'interface web

**Modifier l'interface** :
1. **Éditer** les fichiers source dans `web/` :
   - `web/index.html` : HTML/CSS
   - `web/js/*.js` : Modules JavaScript (par responsabilité)
2. **Build automatique** : `./scripts/nidmi.sh sync`
3. **Tester** : Compiler et uploader vers ESP32

**Workflow de développement complet** :
```bash
# 1. Éditer l'interface
vim web/index.html              # HTML/CSS
vim web/js/pins.js              # Module spécifique

# 2. Build automatique (génère HTML + bundle gzip)
./scripts/nidmi.sh sync

# 3. Compiler et uploader
./scripts/nidmi.sh upload

# 4. Tester dans le navigateur
# Ouvrir http://192.168.4.1 (ou nom.local)
```

**Localisation (langues)** :
```bash
# Build en français (défaut)
./scripts/nidmi.sh sync

# Build en anglais
./scripts/nidmi.sh sync --lang en

# Upload avec interface anglaise
./scripts/nidmi.sh upload --lang en
```

**Note** : La localisation nécessite `jq` installé. Voir `docs/GUIDE_LOCALISATION.md` pour plus de détails.

**Conseils de développement** :
- ✅ **Utilisez des commentaires** `/* */` en JavaScript (supprimés automatiquement)
- ❌ **Évitez les commentaires** `//` (interdits sauf dans URLs)
- ✅ **Éditez toujours** les fichiers dans `web/` (jamais `src/ui_index.cpp` directement)
- ✅ **Testez** après chaque modification importante
- ✅ **Consultez** `docs/MODULARISATION_UI.md` pour la structure complète
- ✅ **Localisation** : Consultez `docs/GUIDE_LOCALISATION.md` pour changer la langue

### Localisation (Multi-langue)

L'interface web supporte plusieurs langues via des fichiers JSON de traduction :

```bash
# Build en français (défaut)
./scripts/nidmi.sh sync

# Build en anglais
./scripts/nidmi.sh sync --lang en

# Upload avec interface anglaise
./scripts/nidmi.sh upload --lang en
```

**Langues supportées** :
- 🇫🇷 Français (défaut)
- 🇬🇧 Anglais

**Fichiers de traduction** : `web/lang/fr.json`, `web/lang/en.json`

**Prérequis** : `jq` doit être installé pour les traductions (voir `docs/GUIDE_LOCALISATION.md`)

**Documentation complète** : Consultez `docs/GUIDE_LOCALISATION.md` pour :
- Comment ajouter une nouvelle langue
- Comment utiliser les placeholders `{{t.xxx}}` dans le HTML/JS
- Dépannage et bonnes pratiques

### Optimisations Appliquées

**Build Process** :
1. Chargement des traductions (si langue autre que français)
2. Remplacement des placeholders `{{t.xxx}}` dans HTML et JS
3. Concaténation des modules JS dans l'ordre correct
4. Génération HTML minimal avec `<script src="/bundle"></script>`
5. Minification JavaScript (suppression commentaires `/* */`)
6. Compression gzip du bundle JS
7. Minification HTML (suppression commentaires + espaces)
8. Conversion en C++ PROGMEM pour ESP32

**Résultats** :
- **HTML source** : ~15.6 KB
- **JS source** : ~43 KB
- **Bundle JS (gzip)** : ~10.4 KB (réduction 75%)
- **Total minifié** : ~26 KB
- **Réduction totale** : 55% par rapport à version monolithique

**Avantages** :
- **Code maintenable** : Modules organisés par responsabilité
- **Mémoire optimisée** : Compression gzip + HTML minimal
- **Workflow simple** : Build automatique via `nidmi.sh`
- **Fiabilité** : Bug encodage résolu avec streaming par chunks

---

Questions, retours ou idées d'amélioration: issues bienvenues.