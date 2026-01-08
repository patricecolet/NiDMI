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

### **3. Multiplexeurs Analogiques (HC4067)** 🔄 **EN DÉVELOPPEMENT**
- **Status** : 🔄 En développement (branche `feature/analog-mux`)
- **Objectif** : Support des multiplexeurs analogiques 16 canaux
- **Fonctionnalités** :
  - ✅ Composant `AnalogMux` implémenté (basé sur Control-Surface)
  - ✅ Support jusqu'à 3 multiplexeurs (MUX0, MUX1, MUX2)
  - ✅ GPIO virtuels : 200-215 (MUX0), 216-231 (MUX1), 232-247 (MUX2)
  - ✅ API REST : `/api/mux/list`, `/api/mux/add`, `/api/mux/delete`
  - ✅ Interface web complète pour configuration
  - ✅ Sauvegarde/chargement NVS automatique
  - ✅ Intégration dans `ComponentManager` et `PinMapper`
  - ✅ Pin EN optionnelle (active LOW)
  - ✅ Discard first reading pour stabilité
  - 🔄 Tests et validation en cours

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
- **Serveur web** : Interface de configuration
- **RTP-MIDI** : Communication sans fil
- **OSC** : Open Sound Control complet
- **Pins configurables** : Potentiomètres, boutons, LEDs
- **API REST** : Configuration via HTTP
- **Stockage NVS** : Configuration persistante
- **ESP32-C3** : Support complet
- **WebSocket** : Synchronisation temps réel

### **🔄 En Développement**
- **Multiplexeurs analogiques** : Support HC4067 (branche `feature/analog-mux`)
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
├── OSCManager.cpp/h          # Gestion OSC ✅
├── DebugManager.cpp/h        # Système de logs 🔄
├── WebAPI.cpp                 # API REST
├── ServerCore.cpp/h           # Cœur du serveur
├── ui_index.cpp/h             # Interface web intégrée
└── components/
    ├── AnalogMux.h            # Multiplexeur analogique 🔄
    ├── Potentiometer.h        # Potentiomètre
    ├── Button.h               # Bouton
    └── Led.h                  # LED
```

### **Nouvelles Classes à Développper**
- **`UsbMidiManager`** : USB-MIDI
- **`TouchManager`** : Touch pins ESP32-S3

## 📝 **Notes de Développement**

### **Multiplexeurs Analogiques (Priorité 1 - En cours)**
- ✅ Composant `AnalogMux` implémenté
- ✅ Intégration dans `ComponentManager`
- ✅ API REST et interface web
- 🔄 Tests et validation nécessaires
- 🔄 Documentation d'utilisation à créer
- **Branche** : `feature/analog-mux`

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

## 📌 **État Actuel du Projet (Mise à jour)**

**Branche active** : `feature/analog-mux`

**Dernière fonctionnalité** : Support des multiplexeurs analogiques HC4067
- Implémentation complète du composant `AnalogMux`
- Intégration dans le système de gestion des composants
- Interface web et API REST fonctionnelles
- En attente de tests et validation

**Prochaines étapes** :
1. Finaliser et tester les multiplexeurs analogiques
2. Corriger le système de debug (priorité haute)
3. Uniformiser l'interface web (boutons/LEDs)
4. Poursuivre le support ESP32-S3

---

*Document mis à jour le : 2024*
*Ordre de priorité actuel : MUX → DEBUG → S3 → USBMIDI → TOUCH*