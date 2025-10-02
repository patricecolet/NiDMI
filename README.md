# ESP32Server (librairie Arduino)

Serveur web simple (HTTP + WebSocket) pour ESP32‑C3/S3, destiné à des ateliers capteurs/actuateurs (musique électroacoustique et actuelle).

- Documentation avancée (MIDI / OSC / Temps réel): consultez `ADVANCED.md`.

## Installation (macOS / Windows / Linux)

### Option A — IDE Arduino (recommandé pour débuter)
1. Ouvrir l'IDE Arduino 2.x.
2. Installer le core ESP32: Outils > Type de carte > Gestionnaire de cartes… → chercher « esp32 » (Espressif Systems) → Installer.
3. Installer les dépendances via le Gestionnaire de bibliothèques:
   - « ESP Async WebServer »
   - « AsyncTCP »
4. Ajouter cette librairie:
   - Soit via « Ajouter la bibliothèque .ZIP… » (si usage local),
   - Soit plus tard via le Library Manager (après publication officielle).
5. Ouvrir: Fichier > Exemples > ESP32Server > esp32server_basic → Téléverser.

Notes:
- L'IDE 2.x sait proposer l'installation des dépendances si la librairie est installée via le Library Manager. Pour une librairie locale/ZIP, installez les 2 libs ci‑dessus manuellement (étape 3) ou utilisez l'option B.

### Option B — Automatique avec arduino-cli (avancé)
- macOS/Linux: `bash scripts/install_deps.sh`
- Windows: `PowerShell -ExecutionPolicy Bypass -File scripts/install_deps.ps1`
Ces scripts installent le core `esp32:esp32` et les librairies « ESP Async WebServer » et « AsyncTCP ».

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
#include <ESP32Server.h>

void setup() {
  Serial.begin(115200);
  
  // Démarrer le serveur avec nom personnalisé
  ESP32Server.begin("MonServeur");
  
  // Attendre la connexion WiFi
  while (!ESP32Server.isConnected()) {
    delay(100);
  }
  
  Serial.println("Serveur prêt !");
  Serial.print("IP: ");
  Serial.println(ESP32Server.getIP());
}

void loop() {
  ESP32Server.update();
}
```

### Configuration via interface web

1. **Connexion** : L'ESP32 crée un point d'accès WiFi `ESP32Server-XXXX`
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

- **`Esp32Server`** : Classe principale, gestion WiFi et serveur web
- **`ComponentManager`** : Gestion des pins et composants
- **`PinMapper`** : Mapping des pins ESP32-C3/S3
- **`RtpMidi`** : Communication MIDI sans fil
- **`WebAPI`** : Interface REST pour la configuration

### Structure des fichiers

```
src/
├── Esp32Server.cpp/h          # Classe principale
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
#define ESP32SERVER_ENABLE_BLE_MIDI
#include <esp32server.h>
```

### Utilisation

```cpp
void setup() {
    esp32server_setup();
    
    // Configuration normale
    esp32server_addButton(2, 1, 60, 1);
    esp32server_addPotentiometer(6, 1, 1, 1);
}

void loop() {
    esp32server_loop();
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

### Compilation

```bash
# Installation des dépendances
./scripts/install_deps.sh

# Compilation
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32C3 examples/esp32server_basic/esp32server_basic.ino
```

### Tests

```bash
# Tests de l'interface web
./scripts/minify_test.sh

# Tests de compilation
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32C3 examples/esp32server_basic/esp32server_basic.ino
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

### 🐛 Limitations OSC (Open Sound Control)

**Problème** : Perte de paquets OSC sur les boutons en WiFi, particulièrement sur les transitions rapides.

**Impact** : 
- Les boutons peuvent ne pas envoyer tous les messages OSC
- Les potentiomètres fonctionnent mieux que les boutons
- RTP-MIDI reste la solution la plus fiable

**Workaround** : 
- Privilégier RTP-MIDI pour les boutons
- Utiliser OSC principalement pour les potentiomètres
- Tester avec des transitions plus lentes

**Status** : Limitation connue, RTP-MIDI recommandé pour la fiabilité.

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

### Minification automatique

L'interface web est automatiquement minifiée pour optimiser l'utilisation de la mémoire flash :

```bash
# Minification de l'interface
./scripts/minify_safe.sh

# Test de la minification
./scripts/minify_test.sh
```

**Optimisations appliquées** :
- ✅ Suppression des commentaires HTML `<!-- -->`
- ✅ Suppression des commentaires JavaScript `/* */`
- ✅ Remplacement des espaces multiples par un seul espace
- ✅ **Réduction de 31%** (46589 → 31110 bytes)

**Avantages** :
- **Code lisible** : Interface développée dans `web/index.html`
- **Mémoire optimisée** : Version minifiée dans le firmware
- **Workflow simple** : Un script pour tout automatiser
- **Sécurité** : Minification sûre qui préserve la fonctionnalité

### Développement de l'interface web

**Modifier l'interface** :
1. **Éditer** `web/index.html` avec votre éditeur préféré
2. **Tester** les modifications dans un navigateur (fichier local)
3. **Minifier** avec `./scripts/minify_safe.sh`
4. **Synchroniser** la bibliothèque : `cp src/ui_index.cpp ~/Documents/Arduino/libraries/esp32server/src/`
5. **Compiler** le firmware pour tester : `arduino-cli compile --fqbn esp32:esp32:esp32c3 examples/esp32server_basic/esp32server_basic.ino`

**Structure de l'interface** :
- **HTML** : Structure des onglets et formulaires
- **CSS** : Styles pour l'interface ESP32-C3 et les pins
- **JavaScript** : Logique des onglets, API calls, gestion des pins

**Workflow de développement complet** :
```bash
# 1. Éditer l'interface
# Modifier web/index.html avec votre éditeur

# 2. Minifier et intégrer
./scripts/minify_safe.sh

# 3. Synchroniser la bibliothèque
cp src/ui_index.cpp ~/Documents/Arduino/libraries/esp32server/src/

# 4. Tester la compilation
arduino-cli compile --fqbn esp32:esp32:esp32c3 examples/esp32server_basic/esp32server_basic.ino
```

**Conseils de développement** :
- **Utilisez des commentaires** `/* */` en JavaScript (supprimés automatiquement)
- **Évitez les commentaires** `//` (peuvent casser la minification)
- **Testez** toujours après minification
- **Sauvegardez** `web/index.html` avant modifications importantes
- **Éditez toujours** `web/index.html` (pas `src/ui_index.cpp` directement)

---

Questions, retours ou idées d'amélioration: issues bienvenues.