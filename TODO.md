# TODO – Plan d'implémentation NiDMI

## 🎯 **Priorités de Développement**

### **0. Modularisation Interface Web** ✅ **TERMINÉ**
- **Status** : ✅ Implémenté et optimisé
- **Réalisations** :
  - ✅ Code JavaScript organisé en 7 modules (core, api, pins, components, websocket, mux, app)
  - ✅ HTML minimal séparé du JavaScript (~15.6 KB)
  - ✅ Compression gzip du bundle JS (75% de réduction)
  - ✅ Route `/bundle` avec Content-Encoding: gzip
  - ✅ Streaming par chunks depuis PROGMEM (résout bug encodage)
  - ✅ Total optimisé : 26 KB (au lieu de 58 KB) → 55% de réduction
  - ✅ Mémoire ESP32 : 91% utilisée (marge suffisante)

### **1. WebSocket Pin Synchronization** ✅ **TERMINÉ**
- **Status** : ✅ Implémenté
- **Objectif** : Synchronisation temps réel des configurations de pins
- **Fonctionnalités** :
  - ✅ Messages WebSocket `PIN_CLICKED` / `PIN_CONFIG`
  - ✅ Valeurs par défaut uniques par pin (A0→CC#1, A1→CC#2, D0→Note 60, etc.)
  - ✅ Gestion des conflits pins (A0↔D0, SDA↔D4, MOSI↔D8)
  - ✅ Grisage automatique des pins de bus (I2C/SPI)
  - ✅ Configuration OSC/Debug par défaut
  - ✅ Compatible avec système NVS existant

### **2. OSC (Open Sound Control)** ✅ **TERMINÉ**
- **Status** : ✅ Implémenté
- **Objectif** : Support complet OSC pour communication réseau
- **Fonctionnalités** :
  - ✅ Envoi OSC (CC, Note, Program Change)
  - ✅ Support broadcast (AP/STA) et IP spécifique
  - ✅ Format OSC configurable : Float (0-1) ou MIDI (3 int)
  - ✅ Configuration par pin avec adresses personnalisées
  - ✅ Interface web complète pour configuration OSC
  - ✅ Réception OSC (contrôle des LEDs)
  - ✅ Configuration via interface web (intégré WebSocket)
  - ✅ Mapping OSC ↔ MIDI

### **3. Multiplexeurs Analogiques (HC4067)** ✅ **TERMINÉ**
- **Status** : ✅ Implémenté
- **Objectif** : Support des multiplexeurs analogiques 16 canaux
- **Fonctionnalités** :
  - ✅ Composant `AnalogMux` implémenté (basé sur Control-Surface)
  - ✅ Support jusqu'à 2 multiplexeurs (MUX0, MUX1)
  - ✅ GPIO virtuels : 200-215 (MUX0), 216-231 (MUX1)
  - ✅ API REST : `/api/mux/list`, `/api/mux/add`, `/api/mux/delete`
  - ✅ Interface web complète pour configuration (intégrée dans modules)
  - ✅ Sauvegarde/chargement NVS automatique
  - ✅ Intégration dans `ComponentManager` et `PinMapper`
  - ✅ Pin EN optionnelle (active LOW)
  - ✅ Discard first reading pour stabilité
  - ✅ Filtrage configurable (intensité 1-10)
  - ✅ Formats OSC : RAW, FLOAT, MIDI

### **4. DEBUG (Système de Logs)**
- **Status** : 🔄 En développement  
- **Objectif** : Système de debug avancé et monitoring
- **Fonctionnalités** :
  - Logs détaillés (MIDI, OSC, pins, erreurs)
  - Interface web de monitoring
  - Niveaux de log configurables
  - Export des logs

### **5. ESP32-S3 (Support Complet)**
- **Status** : 🔄 En développement
- **Objectif** : Support complet de l'ESP32-S3
- **Fonctionnalités** :
  - ✅ Mapping des pins ESP32-S3 (partiellement implémenté)
  - 🔄 Interface web adaptée ESP32-S3
  - 🔄 Optimisations spécifiques S3
  - 🔄 Tests de compatibilité

### **6. USB-MIDI**
- **Status** : 📋 Planifié
- **Objectif** : Connexion USB directe MIDI
- **Fonctionnalités** :
  - Support USB-MIDI natif
  - Configuration via interface web
  - Routage USB ↔ RTP-MIDI
  - Compatibilité macOS/Windows/Linux

### **7. TOUCH PINS (ESP32-S3)**
- **Status** : 📋 Planifié
- **Objectif** : Support des touch pins ESP32-S3
- **Fonctionnalités** :
  - ComponentType::TOUCH
  - Interface tactile intuitive
  - Configuration seuils
  - MIDI Note On/Off via touch

## 📋 **Fonctionnalités Actuelles**

### **✅ Implémenté**
- **Serveur web** : Interface de configuration modulaire et optimisée
- **RTP-MIDI** : Communication sans fil complète
- **OSC** : Open Sound Control complet (Float 0-1 et MIDI 3 int)
- **Pins configurables** : Potentiomètres, boutons, LEDs
- **Multiplexeurs** : Support HC4067 (2 multiplexeurs, 16 canaux chacun)
- **API REST** : Configuration via HTTP
- **WebSocket** : Synchronisation temps réel des configurations
- **Stockage NVS** : Configuration persistante automatique
- **ESP32-C3** : Support complet
- **Compression gzip** : Bundle JavaScript optimisé (55% de réduction totale)
- **Streaming HTML** : Par chunks depuis PROGMEM (fiabilité)

### **🔄 En Développement**
- **DEBUG** : Système de logs avancé (priorité haute - bloquant)
- **ESP32-S3** : Support complet (partiellement implémenté)

### **📋 Planifié**
- **USB-MIDI** : Connexion USB directe
- **Touch pins** : Support ESP32-S3
- **Interface améliorée** : Multi-cartes

## 🐛 **Bugs Connus & À Corriger**

### **✅ RÉSOLU - Bug Encodage HTML Aléatoire**
- **Problème** : "Le flux d'octets était en erreur par rapport à l'encodage de caractères déclaré"
- **Cause identifiée** : Buffer overflow lors de l'envoi HTML depuis PROGMEM
- **Solution** : Streaming par chunks + HTML minimal + route `/bundle` séparée
- **Status** : ✅ Résolu et testé

### **❌ PRIORITÉ HAUTE - Système de Debug**
- **Problème** : Les macros de debug ne fonctionnent pas (pas de logs série)
- **Impact** : Impossible de diagnostiquer les problèmes
- **Détails** :
  - Macros définies dans `nidmi_debug.h`
  - `#define NIDMI_DEBUG_NETWORK 1` ne produit aucun log
  - Les `debug_network()` ne s'affichent pas dans le moniteur série
- **Status** : 🔴 À corriger en priorité absolue

### **❌ PRIORITÉ HAUTE - Grisage Automatique Pins I2C/SPI**
- **Problème** : Les pins I2C/SPI ne se grisent pas automatiquement
- **Impact** : Risque de conflits de configuration
- **Détails** :
  - Clic sur SDA ne grise pas SCL, D4, D5
  - Clic sur MOSI/MISO/SCK ne grise pas les autres pins SPI
  - JavaScript reçoit `PIN_CONFIG:SDA:` (vide)
  - `getDefaultConfig()` retourne vide pour SDA/SCL/MOSI/MISO/SCK
- **Corrections apportées** :
  - ✅ Ajout configs par défaut I2C/SPI dans `getDefaultConfig()`
  - ✅ Ajout création `pcfg['I2C']` et `pcfg['SPI']` dans JavaScript
  - ❌ Non testé - debug ne fonctionne pas
- **Status** : 🟡 En cours - en attente debug fonctionnel

### **❌ Options de Pins - Interface Web**
- **Problème** : Options de configuration incorrectes pour certains types de pins
- **Impact** : Interface confuse, options inappropriées
- **Détails** :
  - Boutons : afficher type MIDI (Note, CC)
  - LEDs : afficher type MIDI (Note, CC)
  - Uniformiser avec potentiomètres
- **Status** : 🟡 À corriger

### **⚠️ Écho MIDI RTP-MIDI**
- **Problème** : Retransmission des messages MIDI
- **Impact** : Boucles potentielles dans le DAW
- **Workaround** : Router sur un autre contrôleur
- **Status** : Bug connu, investigation en cours

## 🔧 **Corrections Interface**

### **Cohérence Types MIDI - Interface Web** ✅ **TERMINÉ**
- **Problème** : Incohérence dans l'affichage des types de messages MIDI
- **Détails** :
  - ✅ **Analog (Potentiomètres)** : Affiche correctement le type MIDI (CC, Note, etc.)
  - ✅ **Digital (Boutons/LEDs)** : Affiche maintenant le type de message MIDI
- **Solution appliquée** :
  - ✅ Affichage du type MIDI pour les boutons (Note On/Off, CC, etc.)
  - ✅ Affichage du type MIDI pour les LEDs (Note On/Off, CC, etc.)
  - ✅ Affichage uniformisé entre analog et digital
- **Status** : ✅ Terminé

## 📊 **Roadmap des Versions**

### **v0.1.0** - Version Initiale ✅ **TERMINÉE**
- Serveur web fonctionnel
- RTP-MIDI basique
- Configuration pins simple

### **v0.2.0** - OSC Support ✅ **TERMINÉE**
- ✅ Implémentation OSC complète (Float + MIDI)
- ✅ Interface web OSC
- ✅ Mapping OSC ↔ MIDI
- ✅ Broadcast AP/STA
- ✅ Réception OSC (LEDs)

### **v0.2.1** - Optimisation Interface Web ✅ **TERMINÉE**
- ✅ Modularisation JavaScript (7 modules)
- ✅ Compression gzip (55% de réduction)
- ✅ HTML minimal + route `/bundle`
- ✅ Streaming par chunks (bug encodage résolu)

### **v0.3.0** - Multiplexeurs & Optimisations ✅ **TERMINÉE**
- ✅ Support HC4067 (2 multiplexeurs)
- ✅ Interface web multiplexeurs
- ✅ GPIO virtuels (200-231)
- ✅ Formats OSC multiples (RAW, FLOAT, MIDI)

### **v0.4.0** - Debug & Monitoring 🔄 **EN DÉVELOPPEMENT**
- 🔄 Système de logs avancé (priorité haute)
- 📋 Interface de monitoring
- 📋 Export des logs
- 📋 Niveaux de log configurables

### **v0.5.0** - ESP32-S3 Complet 📋 **PLANIFIÉ**
- 🔄 Support ESP32-S3 (partiellement)
- 📋 Interface adaptée S3
- 📋 Touch pins ESP32-S3
- 📋 Optimisations multi-cœurs

### **v0.6.0** - USB-MIDI 📋 **PLANIFIÉ**
- 📋 Support USB-MIDI natif
- 📋 Configuration USB
- 📋 Routage USB ↔ RTP-MIDI
- 📋 Compatibilité macOS/Windows/Linux

## 🔧 **Développement Technique**

### **Architecture Actuelle**
```
src/
├── NiDMIServer.cpp/h          # Classe principale
├── ComponentManager.cpp/h      # Gestion des composants ✅
├── PinMapper.cpp/h            # Mapping des pins ✅
├── RtpMidi.cpp/h             # RTP-MIDI ✅
├── OSCManager.cpp/h          # Gestion OSC ✅
├── DebugManager.cpp/h        # Système de logs 🔄 (bloquant)
├── WebAPI.cpp                 # API REST + routes / et /bundle ✅
├── ServerCore.cpp/h           # Cœur du serveur ✅
├── ui_index.cpp/h             # HTML minimal (PROGMEM) ✅
├── ui_bundle.h                # Bundle JS gzipé (PROGMEM) ✅
└── components/
    ├── AnalogMux.h            # Multiplexeur analogique ✅
    ├── Potentiometer.h        # Potentiomètre ✅
    ├── Button.h               # Bouton ✅
    └── Led.h                  # LED ✅

web/
├── index.html                 # HTML minimal (source de vérité) ✅
├── app.js                     # Code JS principal résiduel ✅
└── js/
    ├── core.js                # Utilitaires de base ✅
    ├── api.js                 # Appels API ✅
    ├── pins.js                # Gestion des pins ✅
    ├── components.js          # Fonctions génériques ✅
    ├── websocket.js           # WebSocket temps réel ✅
    └── mux.js                 # Multiplexeurs ✅
```

### **Nouvelles Classes à Développper**
- **`UsbMidiManager`** : USB-MIDI
- **`TouchManager`** : Touch pins ESP32-S3

## 📝 **Notes de Développement**

### **Modularisation Interface Web (Terminé)**
- ✅ JavaScript organisé en 7 modules logiques
- ✅ HTML minimal avec route `/bundle` séparée
- ✅ Compression gzip (75% réduction JS, 55% total)
- ✅ Streaming par chunks depuis PROGMEM
- ✅ Bug encodage aléatoire résolu
- ✅ Build automatique intégré dans `nidmi.sh`
- ✅ Documentation complète dans `docs/MODULARISATION_UI.md`

### **Multiplexeurs Analogiques (Terminé)**
- ✅ Composant `AnalogMux` implémenté
- ✅ Intégration dans `ComponentManager`
- ✅ API REST et interface web (module `mux.js`)
- ✅ Support 2 multiplexeurs (GPIO virtuels 200-231)
- ✅ Formats OSC multiples (RAW, FLOAT, MIDI)
- ✅ Filtrage configurable (intensité 1-10)
- ✅ Documentation dans `docs/GUIDE_IMPLÉMENTATION_COMPOSANTS.md`

### **DEBUG (Priorité 1 - BLOQUANT)**
- ❌ Système de logs actuel ne fonctionne pas
- 🔄 Système de logs avec niveaux (CRITICAL, ERROR, WARNING, INFO, DEBUG)
- 📋 Interface web de monitoring
- 📋 Export et rotation des logs
- 📋 Logs structurés (JSON) pour analyse

### **ESP32-S3 (Priorité 2)**
- 🔄 Mapping des pins ESP32-S3 (partiellement implémenté)
- 📋 Adapter l'interface web (dessin du board)
- 📋 Tester la compatibilité complète
- 📋 Optimiser les performances (multi-cœurs si nécessaire)

### **Nouveaux Composants (Priorité 3)**
- 📋 Encodeurs rotatifs (2 pins digitales)
- 📋 Touch pins ESP32-S3 (1 pin)
- 📋 Capteurs I2C (ex: accéléromètre)
- 📋 Voir `docs/GUIDE_IMPLÉMENTATION_COMPOSANTS.md` pour guide complet

### **USB-MIDI (Priorité 4)**
- 📋 Bibliothèque USB-MIDI (ESP32-USB ou TinyUSB)
- 📋 Configuration via interface web
- 📋 Routage intelligent USB ↔ RTP-MIDI
- 📋 Compatibilité macOS/Windows/Linux

---

## 📌 **État Actuel du Projet (Mise à jour)**

**Branche active** : `main`

**Dernières fonctionnalités terminées** :
1. ✅ **Modularisation Interface Web** : Code JavaScript organisé en 7 modules, HTML minimal, compression gzip (55% de réduction)
2. ✅ **Multiplexeurs HC4067** : Support complet de 2 multiplexeurs avec interface web et API REST
3. ✅ **Bug encodage résolu** : Streaming par chunks depuis PROGMEM
4. ✅ **OSC complet** : Float 0-1 et MIDI 3 int, broadcast AP/STA, réception pour LEDs

**Prochaines étapes** (ordre de priorité) :
1. **🔴 PRIORITÉ HAUTE** : Corriger le système de debug (bloquant pour développement)
2. **🟡 PRIORITÉ MOYENNE** : Finaliser support ESP32-S3 (interface web, tests)
3. **🟢 PRIORITÉ BASSE** : Nouveaux composants (encodeurs, touch pins) - voir guide
4. **🟢 PRIORITÉ BASSE** : USB-MIDI (planifié v0.6.0)

**Documentation disponible** :
- `docs/MODULARISATION_UI.md` : Guide complet de la modularisation
- `docs/ETAT_ACTUEL.md` : État actuel détaillé du projet
- `docs/GUIDE_IMPLÉMENTATION_COMPOSANTS.md` : Guide pour stagiaires (nouveaux composants)
- `docs/ADVANCED.md` : Guide avancé (architecture temps réel, optimisations)

---

*Document mis à jour le : Janvier 2025*
*Ordre de priorité actuel : DEBUG → S3 → NOUVEAUX_COMPOSANTS → USBMIDI*