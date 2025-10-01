# TODO – Plan d'implémentation ESP32Server

## 🎯 **Priorités de Développement**

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

### **3. DEBUG (Système de Logs)**
- **Status** : 🔄 En développement  
- **Objectif** : Système de debug avancé et monitoring
- **Fonctionnalités** :
  - Logs détaillés (MIDI, OSC, pins, erreurs)
  - Interface web de monitoring
  - Niveaux de log configurables
  - Export des logs

### **4. ESP32-S3 (Support Complet)**
- **Status** : 🔄 En développement
- **Objectif** : Support complet de l'ESP32-S3
- **Fonctionnalités** :
  - Interface web adaptée ESP32-S3
  - Mapping des pins ESP32-S3
  - Optimisations spécifiques S3
  - Tests de compatibilité

### **4. USB-MIDI**
- **Status** : 📋 Planifié
- **Objectif** : Connexion USB directe MIDI
- **Fonctionnalités** :
  - Support USB-MIDI natif
  - Configuration via interface web
  - Routage USB ↔ RTP-MIDI
  - Compatibilité macOS/Windows/Linux

### **5. TOUCH PINS (ESP32-S3)**
- **Status** : 📋 Planifié
- **Objectif** : Support des touch pins ESP32-S3
- **Fonctionnalités** :
  - ComponentType::TOUCH
  - Interface tactile intuitive
  - Configuration seuils
  - MIDI Note On/Off via touch

## 📋 **Fonctionnalités Actuelles**

### **✅ Implémenté**
- **Serveur web** : Interface de configuration
- **RTP-MIDI** : Communication sans fil
- **Pins configurables** : Potentiomètres, boutons, LEDs
- **API REST** : Configuration via HTTP
- **Stockage NVS** : Configuration persistante
- **ESP32-C3** : Support complet

### **🔄 En Développement**
- **OSC** : Open Sound Control
- **DEBUG** : Système de logs avancé
- **ESP32-S3** : Support complet

### **📋 Planifié**
- **USB-MIDI** : Connexion USB directe
- **Touch pins** : Support ESP32-S3
- **Interface améliorée** : Multi-cartes

## 🐛 **Bugs Connus & À Corriger**

### **❌ PRIORITÉ HAUTE - Système de Debug**
- **Problème** : Les macros de debug ne fonctionnent pas (pas de logs série)
- **Impact** : Impossible de diagnostiquer les problèmes
- **Détails** :
  - Macros définies dans `esp32server_debug.h`
  - `#define ESP32SERVER_DEBUG_NETWORK 1` ne produit aucun log
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

### **Cohérence Types MIDI - Interface Web**
- **Problème** : Incohérence dans l'affichage des types de messages MIDI
- **Détails** :
  - ✅ **Analog (Potentiomètres)** : Affiche correctement le type MIDI (CC, Note, etc.)
  - ❌ **Digital (Boutons/LEDs)** : N'affiche pas le type de message MIDI
- **Impact** : Interface confuse, manque de cohérence visuelle
- **Solution** :
  - Afficher le type MIDI pour les boutons (Note On/Off, CC, etc.)
  - Afficher le type MIDI pour les LEDs (Note On/Off, CC, etc.)
  - Uniformiser l'affichage entre analog et digital
- **Status** : 🔄 À corriger (priorité haute)

## 📊 **Roadmap des Versions**

### **v0.2.0** - OSC Support
- Implémentation OSC complète
- Interface web OSC
- Mapping OSC ↔ MIDI

### **v0.3.0** - Debug & Monitoring  
- Système de logs avancé
- Interface de monitoring
- Export des logs

### **v0.4.0** - ESP32-S3 Complet
- Support complet ESP32-S3
- Interface adaptée S3
- Touch pins ESP32-S3

### **v0.5.0** - USB-MIDI
- Support USB-MIDI natif
- Configuration USB
- Routage USB ↔ RTP-MIDI

## 🔧 **Développement Technique**

### **Architecture Actuelle**
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

### **Nouvelles Classes à Développper**
- **`OscManager`** : Gestion OSC
- **`DebugManager`** : Système de logs
- **`UsbMidiManager`** : USB-MIDI
- **`TouchManager`** : Touch pins ESP32-S3

## 📝 **Notes de Développement**

### **OSC (Priorité 1)**
- Utiliser la bibliothèque OSC standard
- Interface web pour configuration
- Mapping bidirectionnel OSC ↔ MIDI

### **DEBUG (Priorité 2)**
- Système de logs avec niveaux
- Interface web de monitoring
- Export et rotation des logs

### **ESP32-S3 (Priorité 3)**
- Adapter l'interface web
- Tester la compatibilité
- Optimiser les performances

### **USB-MIDI (Priorité 4)**
- Bibliothèque USB-MIDI
- Configuration via interface
- Routage intelligent

### **Touch Pins (Priorité 5)**
- ComponentType::TOUCH
- Interface tactile
- Configuration seuils

---

*Document mis à jour le : $(date)*
*Ordre de priorité : OSC → DEBUG → S3 → USBMIDI → TOUCH*