# ESP32Server (librairie Arduino)

Serveur web simple (HTTP + WebSocket) pour ESP32‑C3/S3, destiné à des ateliers capteurs/actuateurs (musique électroacoustique et actuelle).

- Documentation avancée (MIDI / OSC / Temps réel): consultez `ADVANCED.md`.

## Installation (macOS / Windows / Linux)

### Option A — IDE Arduino (recommandé pour débuter)
1. Ouvrir l’IDE Arduino 2.x.
2. Installer le core ESP32: Outils > Type de carte > Gestionnaire de cartes… → chercher « esp32 » (Espressif Systems) → Installer.
3. Installer les dépendances via le Gestionnaire de bibliothèques:
   - « ESP Async WebServer »
   - « AsyncTCP »
4. Ajouter cette librairie:
   - Soit via « Ajouter la bibliothèque .ZIP… » (si usage local),
   - Soit plus tard via le Library Manager (après publication officielle).
5. Ouvrir: Fichier > Exemples > ESP32Server > esp32server_basic → Téléverser.

Notes:
- L’IDE 2.x sait proposer l’installation des dépendances si la librairie est installée via le Library Manager. Pour une librairie locale/ZIP, installez les 2 libs ci‑dessus manuellement (étape 3) ou utilisez l’option B.

### Option B — Automatique avec arduino-cli (avancé)
- macOS/Linux: `bash scripts/install_deps.sh`
- Windows: `PowerShell -ExecutionPolicy Bypass -File scripts/install_deps.ps1`
Ces scripts installent le core `esp32:esp32` et les librairies « ESP Async WebServer » et « AsyncTCP ».

## Utilisation

### Exemple de base
- La classe `Esp32Server` démarre un point d'accès WiFi et un serveur HTTP/WS.
- Exemple fourni: `examples/esp32server_basic/esp32server_basic.ino`
  - SSID par défaut: `ESP32-Server`, mot de passe: `esp32pass`
  - Page: `http://192.168.4.1/`
  - API: `GET /api/info`, `GET /api/ip`
  - WebSocket: `ws://192.168.4.1/ws`

Personnaliser les identifiants dans l'exemple:
```cpp
srv.begin("MonSSID","MonMotDePasse");
```

### Exemple RTP-MIDI avancé
- Exemple complet: `examples/rtpmidi/rtpmidi_test/rtpmidi_test.ino`
- Interface web avancée avec onglets (Statut, Connection, Pins)
- Configuration RTP-MIDI, OSC, mDNS
- Gestion des pins ESP32-C3 avec interface graphique
- API endpoints: `/api/status`, `/api/rtp/status`, `/api/osc/status`, `/api/mdns`, `/api/pins/caps`

## Interface web avancée

### Onglets disponibles
- **Statut** : Informations réseau (AP/STA), connexions
- **Connection** : Configuration serveur, OSC, Wi-Fi STA
- **Pins** : Interface graphique ESP32-C3, configuration des pins

### Configuration des pins
- **Interface graphique** : Diagramme ESP32-C3 cliquable
- **Rôles** : Potentiomètre, Bouton, LED, I2C, SPI, UART
- **RTP-MIDI** : Configuration par pin (Note, CC, Program Change, etc.)
- **OSC** : Adresses personnalisées par pin
- **Debug** : Console de débogage configurable

### API endpoints
- `GET /api/status` : Statut réseau
- `GET /api/osc/status` : Configuration OSC
- `POST /api/osc` : Configurer OSC
- `GET /api/mdns/status` : Nom du serveur
- `POST /api/mdns` : Changer le nom du serveur
- `GET /api/pins/caps` : Capacités des pins
- `POST /api/pins/set` : Configuration d'un pin

## Guide AP/STA (débutants)
Objectif: garder un **AP** (point d’accès) pour l’accès direct ET connecter la carte à votre **Wi‑Fi domestique (STA)**.

1) Démarrer l’AP (déjà fait par défaut dans l’exemple):
```cpp
const char* apSsid = "ESP32-Server";
const char* apPass = "esp32pass";
srv.begin(apSsid, apPass);
```
- Connectez votre ordinateur/smartphone à ce réseau pour accéder à `http://192.168.4.1/`.

2) (Optionnel) Configurer une IP statique pour le mode STA:
```cpp
// IP fixe sur votre réseau local (adapter aux valeurs de votre LAN)
srv.setStaticStaIp(IPAddress(192,168,1,50), IPAddress(192,168,1,1), IPAddress(255,255,255,0));
```

3) Se connecter à votre Wi‑Fi (STA):
```cpp
const char* staSsid = "VotreSSID";
const char* staPass = "VotrePass";
srv.connectSta(staSsid, staPass);
```

4) Vérifier les adresses IP (AP et STA):
- Moniteur série: l’exemple affiche périodiquement les IP.
- HTTP: `GET /api/ip` renvoie un JSON avec les IP AP et STA, par ex:
```json
{"ap":"192.168.4.1","sta":"192.168.1.50"}
```

Conseils:
- Commencez avec AP seul (accès garanti à `192.168.4.1`), puis ajoutez la STA.
- En STA, l’IP peut être `0.0.0.0` tant que la connexion n’est pas établie.

## Navigateurs (Chrome/Brave vs Firefox)
- Chrome/Brave peuvent bloquer l’accès HTTP non‑sécurisé à des IP locales (ou appliquer des politiques strictes) → la page peut être inaccessible alors que le ping répond.
- Recommandation: utiliser **Firefox** pour un accès direct par IP locale.
- Alternatives:
  - mDNS: accéder via `http://esp32server.local/` (macOS/Windows OK; Linux nécessite `avahi-daemon`).
  - Dans Brave/Chrome: désactiver temporairement Shields/“Insecure content” pour l’IP concernée (paramètres du site).
  - Utiliser l’AP `http://192.168.4.1/` pour la configuration initiale, puis repasser en STA.

## OTA (mise à jour de firmware par WiFi)
1. Compiler et exporter le binaire (`.bin`) dans l’IDE: Croquis > Exporter les binaires compilés.
2. Envoyer le binaire à l’ESP32 (carte en AP, IP par défaut `192.168.4.1`):
   - macOS/Linux:
     ```bash
     curl -X POST --data-binary @firmware.bin http://192.168.4.1/update
     ```
   - Windows (PowerShell):
     ```powershell
     Invoke-WebRequest -Uri http://192.168.4.1/update -Method POST -InFile "C:\\chemin\\firmware.bin" -ContentType application/octet-stream
     ```
3. La carte renvoie `204` et redémarre sur le nouveau firmware.

Conseil: gardez un firmware qui inclut l’endpoint `/update` pour pouvoir ré‑flasher à distance.

## Compatibilité cartes
- ESP32‑C3: « ESP32C3 Dev Module »
- ESP32‑S3: « ESP32S3 Dev Module »

## Rôles et messages RTP‑MIDI / OSC

### Potentiomètre (A0–A3) [émission]
- CC: CC# configurable; canal.
- Pitch Bend: plage standard; canal.
- Aftertouch (Channel): 0–127; canal.
- Note + vélocité (note fixe): note configurable; vélocité issue du potard; canal.
- Note (balayage): note min/max; vélocité fixe; envoi seulement si la note change; canal.

### Bouton (D0..D6) [émission]
- Note (On/Off): note configurable; canal; vélocité configurable.
- CC (0/127): CC# configurable; canal; valeurs ON/OFF configurables.
- Program Change: PC configurable; canal.
- Clock: impulsion → tick 24 ppq; pas de canal.
- Tap Tempo: impulsions → calcul BPM; envoi Clock (Start/Stop/Continue) selon logique; pas de canal.

### LED (D7–D10) [réception]
- Mode LED On/Off (suivi MIDI/OSC selon config).
- Mode LED PWM (intensité via CC ou vélocité de note selon mapping).

Notes:
- Clock et Tap Tempo n’ont pas de canal MIDI.
- Les réglages sont spécifiques à chaque rôle.

## Dépendances
- Core ESP32 (Espressif Systems)
- Bibliothèques: « ESP Async WebServer », « AsyncTCP »
- Déclarées aussi dans `library.properties` (clé `depends`) pour le Library Manager.

## Publication au Library Manager (optionnel)
- Préparer un dépôt public (GitHub) avec `library.properties`, `src/`, `examples/`.
- Versionner (tags sémantiques: v0.0.1, v0.0.2, …).
- Soumettre la librairie au registre: dépôt `arduino/library-registry` (ouvrir une issue avec l’URL du repo).
- Après intégration, l’IDE proposera l’installation automatique des dépendances.

## UI HTML externe, injectée et minifiée (idée)
- Maintenir la page UI en dehors du C++ (ex: `web/index.html`).
- Un script de build transforme ce HTML en chaîne C++ compacte pour `ui_index.cpp`:
  - supprime commentaires, logs/debug, espaces inutiles/minifie
  - échappe correctement les guillemets et backticks
  - génère un header/source (ex: `ui_index.h/.cpp`) avec `const char INDEX_HTML[] PROGMEM = R"(... )";`

### Workflow de développement UI

**Structure des fichiers** :
- `web/index.html` : Interface utilisateur complète (HTML, CSS, JavaScript)
- `src/ui_index.cpp` : Version minifiée injectée dans le firmware
- `scripts/minify_safe.sh` : Script de minification automatique

**Workflow** :
1. **Modifier l'interface** : Éditer `web/index.html`
2. **Minifier et injecter** : `./scripts/minify_safe.sh`
3. **Compiler** : Le firmware utilise la version minifiée

**Minification automatique** :
```bash
# Minifier l'interface
./scripts/minify_safe.sh

# Résultat
# 📊 Résultats:
#   Taille HTML:      46589 bytes
#   Taille C++:       31110 bytes
#   Réduction:        31%
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
4. **Compiler** le firmware pour tester

**Structure de l'interface** :
- **HTML** : Structure des onglets et formulaires
- **CSS** : Styles pour l'interface ESP32-C3 et les pins
- **JavaScript** : Logique des onglets, API calls, gestion des pins

**Conseils de développement** :
- **Utilisez des commentaires** `/* */` en JavaScript (supprimés automatiquement)
- **Évitez les commentaires** `//` (peuvent casser la minification)
- **Testez** toujours après minification
- **Sauvegardez** `web/index.html` avant modifications importantes

---
Questions, retours ou idées d’amélioration: issues bienvenues.
