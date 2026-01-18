# Guide d'Implémentation de Nouveaux Composants

Ce guide s'adresse aux stagiaires et développeurs qui souhaitent ajouter de nouveaux types de composants à NiDMI (ex: encodeurs rotatifs, capteurs touch, etc.).

## 📋 Table des matières

1. [Architecture des composants](#architecture-des-composants)
2. [Familles de composants](#familles-de-composants)
3. [Composants simples vs complexes](#composants-simples-vs-complexes)
4. [Création d'un composant simple](#création-dun-composant-simple)
5. [Création d'un composant complexe](#création-dun-composant-complexe)
6. [Intégration dans l'UI](#intégration-dans-lui)
7. [Bonnes pratiques](#bonnes-pratiques)
8. [TODO - Prochaines étapes](#todo---prochaines-étapes)

## 🏗️ Architecture des composants

### Structure des fichiers (cible)

```
src/
├── components/                    # Définitions des composants
│   ├── ComponentTypes.h           # Types de base (ComponentType, ComponentConfig, ComponentState)
│   ├── ComponentDefinition.h      # Structure de définition pour l'UI + ComponentFamily enum
│   ├── ComponentRegistry.h/cpp    # Registre central des composants
│   ├── ValidationRegistry.h/cpp   # Registre des validators
│   │
│   ├── basic/                     # Famille BASIC (composants simples)
│   │   ├── PotentiometerDef.h     # Potentiomètre analogique
│   │   ├── ButtonDef.h            # Bouton poussoir
│   │   └── LedDef.h               # LED (sortie PWM)
│   │
│   ├── multiplexer/               # Famille MULTIPLEXER
│   │   └── MuxDef.h               # HC4067, HC4051 (via héritage MuxBase)
│   │
│   ├── encoder/                   # Famille ENCODER (futur)
│   │   └── QuadratureDef.h
│   │
│   └── display/                   # Famille DISPLAY (futur)
│       └── SSD1306Def.h
│
├── processors/                    # Logique de traitement
│   ├── ProcessorRegistry.h/cpp    # Dispatch dynamique
│   ├── PotentiometerProcessor.h/cpp
│   ├── ButtonProcessor.h/cpp
│   └── LedProcessor.h/cpp
├── managers/                      # Gestion des composants complexes
│   ├── ComponentManager.h/cpp     # Manager central
│   ├── MuxManager.h/cpp           # Manager spécifique MUX
│   └── MuxValidator.h/cpp         # Validation MUX
└── hardware/
    └── AnalogMux.h                # Driver hardware MUX
```

## 🏠 Familles de composants

Les composants sont organisés en **familles** (categories). Chaque famille a son propre dossier.

### Enum ComponentFamily

```cpp
// src/components/ComponentDefinition.h
enum class ComponentFamily : uint8_t {
    BASIC = 0,        // Potentiomètre, Bouton, LED
    MULTIPLEXER = 1,  // HC4067, HC4051, CD4052...
    ENCODER = 2,      // Encodeurs rotatifs
    DISPLAY = 3       // Écrans OLED, LCD
    // Ajouter de nouvelles familles ici
};
```

### Structure dans l'UI

Le frontend affiche **deux menus déroulants** :

1. **Famille** : Basic, Multiplexeur, Encodeur, Écran...
2. **Composant** : liste filtrée selon la famille sélectionnée

```
[Famille ▼]           [Composant ▼]
   Basic                 Potentiomètre
   Multiplexeur          Bouton
   Encodeur              LED
   Écran
```

Quand on sélectionne "Multiplexeur" :
```
[Famille ▼]           [Composant ▼]
   Multiplexeur          HC4067 (16 canaux)
                         HC4051 (8 canaux) [grisé si non implémenté]
```

### Avantages

1. **Organisation claire** : un dossier par famille
2. **Extensibilité** : ajouter un composant = créer un fichier dans le bon dossier
3. **UI cohérente** : toujours 2 menus
4. **Pas de format `id:variant`** : chaque modèle a son propre `id`

### Types et enums

```cpp
// src/components/ComponentTypes.h
enum class ComponentType : uint8_t {
    POTENTIOMETER = 0,  // Potentiomètre analogique
    BUTTON = 1,         // Bouton poussoir
    LED = 2,            // LED (sortie)
    MUX = 3             // Multiplexeur analogique
    // Ajouter votre nouveau type ici
};

enum class PinType : uint8_t {
    PIN_ANALOG = 0,          // Pin analogique (ADC)
    PIN_DIGITAL = 1,         // Pin digitale
    PIN_ANALOG_OR_DIGITAL = 2,
    PIN_PWM = 3              // Pin avec PWM
};
```

### Structure de définition

Chaque composant a une définition qui décrit ses caractéristiques pour l'UI :

```cpp
// src/components/ComponentDefinition.h
struct ComponentDefinition {
    const char* id;              // "potentiometer", "hc4067", etc. (unique)
    const char* displayName;     // "Potentiomètre", "HC4067 (16 canaux)"
    const char* icon;            // Icône (optionnel)
    const char* cardId;          // ID de la carte HTML ("cardPot", "cardMux")
    ComponentFamily family;      // BASIC, MULTIPLEXER, ENCODER, DISPLAY
    const char* familyName;      // "Basic", "Multiplexeur" (pour l'UI)
    ComponentType type;          // Type enum
    PinType pinType;             // Type de pin requis
    bool implemented;            // true = disponible, false = grisé
    // Note: isComplex supprimé - utiliser additionalPinCount > 0 pour détecter un composant avec pins additionnelles
    bool supportsMidi;           // true = peut envoyer/recevoir MIDI
    bool supportsOsc;            // true = peut envoyer/recevoir OSC
    
    // Pins additionnelles (pour composants avec additionalPinCount > 0)
    uint8_t additionalPinCount;  // 0 pour composant simple, >0 pour composant avec pins additionnelles
    AdditionalPinDef additionalPins[MAX_ADDITIONAL_PINS];
    
    // Messages MIDI supportés
    uint8_t midiMessageCount;
    MidiMessageDef midiMessages[MAX_MIDI_MESSAGES];
};
```

## 📦 Composants simples vs complexes

| Aspect | Simple | Complexe |
|--------|--------|----------|
| **Exemples** | Potentiomètre, Bouton, LED | MUX, Matrice de boutons |
| **Pins** | 1 seule pin | Plusieurs pins |
| **Fichiers** | Définition + Processor | Définition + Processor + Manager + Validator |
| **Gestion** | ComponentManager | Manager dédié (ex: MuxManager) |

## 🔧 Création d'un composant simple

### Exemple : Encoder rotatif

#### Étape 1 : Ajouter le type dans `ComponentTypes.h`

```cpp
enum class ComponentType : uint8_t {
    POTENTIOMETER = 0,
    BUTTON = 1,
    LED = 2,
    MUX = 3,
    ENCODER = 4  // ← Nouveau
};
```

#### Étape 2 : Créer la définition dans `components/input/`

```cpp
// src/components/input/EncoderDef.h
#pragma once

#include "../ComponentDefinition.h"

namespace Components {

struct Encoder {
    // Identifiants
    static constexpr const char* ID = "encoder";
    static constexpr const char* DISPLAY_NAME = "Encodeur";
    
    // Configuration
    static constexpr ComponentType TYPE = ComponentType::ENCODER;
    static constexpr PinType PIN_TYPE = PinType::PIN_DIGITAL;
    static constexpr bool IMPLEMENTED = true;
    // Note: IS_COMPLEX supprimé - utiliser additionalPinCount > 0 dans la définition
    
    // Valeurs par défaut
    static constexpr uint8_t DEFAULT_CC = 1;
    static constexpr uint8_t DEFAULT_CHANNEL = 1;
    
    // Validation inline
    static bool validate(uint8_t gpio) {
        return gpio < 48;  // N'importe quel GPIO valide
    }
    
    // Créer la définition
    static ComponentDefinition createDefinition() {
        ComponentDefinition def;
        def.id = ID;
        def.displayName = DISPLAY_NAME;
        def.type = TYPE;
        def.pinType = PIN_TYPE;
        def.implemented = IMPLEMENTED;
        // Note: isComplex supprimé - définir additionalPinCount > 0 pour les composants avec pins additionnelles
        def.additionalPinCount = 0;  // 0 pour composant simple
        return def;
    }
};

} // namespace Components
```

#### Étape 3 : Créer le processor dans `processors/`

```cpp
// src/processors/EncoderProcessor.h
#pragma once

#include "../components/ComponentTypes.h"

class MidiSender;
class OSCQueue;

class EncoderProcessor {
public:
    static void process(
        ComponentConfig& config,
        ComponentState& state,
        MidiSender* midi_sender,
        OSCQueue& osc_queue
    );
    
private:
    static constexpr uint32_t DEBOUNCE_MS = 5;
};
```

```cpp
// src/processors/EncoderProcessor.cpp
#include "EncoderProcessor.h"
#include "../midi/MidiSender.h"
#include "../osc/OSCQueue.h"

void EncoderProcessor::process(
    ComponentConfig& config,
    ComponentState& state,
    MidiSender* midi_sender,
    OSCQueue& osc_queue
) {
    // Votre logique ici
    // Lire les pins, détecter rotation, envoyer MIDI/OSC
    
    // Exemple simplifié :
    uint16_t value = /* lire l'encodeur */;
    
    if (value != state.last_value) {
        state.last_value = value;
        
        // Envoyer MIDI
        if (config.flags & 0x01 && midi_sender) {
            midi_sender->sendControlChange(
                config.midi_channel,
                config.midi_param,
                value & 0x7F
            );
        }
        
        // Envoyer OSC
        if (config.flags & 0x02) {
            osc_queue.enqueueFloat(config.osc_address, value / 127.0f);
        }
    }
}
```

#### Étape 4 : Enregistrer dans `ProcessorRegistry`

```cpp
// src/processors/ProcessorRegistry.cpp
#include "EncoderProcessor.h"

void ProcessorRegistry::processComponent(...) {
    switch (config.type) {
        // ... cas existants ...
        case ComponentType::ENCODER:
            EncoderProcessor::process(config, state, midi_sender, osc_queue);
            break;
    }
}
```

#### Étape 5 : Enregistrer dans `ComponentRegistry`

```cpp
// src/components/ComponentRegistry.cpp
void ComponentRegistry::init() {
    // ... composants existants ...
    
    // Encodeur
    definitions_.push_back({
        "encoder",
        "Encodeur",
        nullptr,
        ComponentType::ENCODER,
        PinType::PIN_DIGITAL,
        true,     // implémenté
        false     // simple
    });
}
```

#### Étape 6 : Ajouter le validator

```cpp
// src/components/ValidationRegistry.cpp
void ValidationRegistry::init() {
    // ... validators existants ...
    
    registerValidator("encoder", [](uint8_t gpio, const void*) {
        return gpio < 48;  // Validation simple
    });
}
```

## 🔧 Création d'un composant complexe

### Exemple : Matrice de boutons

Un composant complexe nécessite :
1. **Définition** (`components/input/MatrixDef.h`)
2. **Manager** (`managers/MatrixManager.h/cpp`)
3. **Validator** (`managers/MatrixValidator.h/cpp`)
4. **Processor** (optionnel, peut être intégré au Manager)

#### Structure

```cpp
// src/components/input/MatrixDef.h
struct Matrix {
    static constexpr const char* ID = "matrix";
    static constexpr const char* DISPLAY_NAME = "Matrice";
    static constexpr ComponentType TYPE = ComponentType::MATRIX;
    // Note: IS_COMPLEX supprimé - définir additionalPinCount > 0 dans createDefinition()
    
    static constexpr uint8_t MAX_ROWS = 8;
    static constexpr uint8_t MAX_COLS = 8;
};

// src/managers/MatrixValidator.h
class MatrixValidator {
public:
    struct ValidationResult {
        bool valid;
        String error_message;
    };
    
    static ValidationResult validatePins(
        const uint8_t* row_pins, uint8_t num_rows,
        const uint8_t* col_pins, uint8_t num_cols
    );
};

// src/managers/MatrixManager.h
class MatrixManager {
public:
    bool addMatrix(...);
    bool removeMatrix(...);
    void update();
    // ...
};
```

## 🌐 Intégration dans l'UI

### API Backend

Les composants sont exposés au frontend via l'API :

```
GET /api/components/definitions
→ [
    {"id":"potentiometer","displayName":"Potentiomètre","pinType":0,"implemented":true,"additionalPinCount":0},
    {"id":"button","displayName":"Bouton","pinType":1,"implemented":true,"additionalPinCount":0},
    ...
  ]
```

### Frontend

Le frontend utilise ces définitions pour générer dynamiquement les options dans l'UI.

Les composants avec `implemented: false` sont affichés en grisé.

## ✅ Bonnes pratiques

### Performance

1. **Pas de `delay()` dans les processors**
   ```cpp
   // ❌ MAUVAIS
   void process(...) { delay(10); }
   
   // ✅ BON
   void process(...) {
       if (millis() - state.last_time < 10) return;
       state.last_time = millis();
   }
   ```

2. **Filtrage pour l'analogique**
   ```cpp
   float filtered = alpha * raw + (1.0f - alpha) * state.filtered;
   ```

3. **Anti-rebond pour le digital**
   ```cpp
   if (millis() - state.last_change_time < DEBOUNCE_MS) return;
   ```

### Mémoire

1. **Pas de `malloc/free` dans `loop()`**
2. **Utiliser des buffers statiques**
3. **Éviter les `String` Arduino (préférer `char[]`)**

### MIDI/OSC

1. **Toujours utiliser `MidiSender`**
2. **Vérifier les flags avant d'envoyer**
   ```cpp
   if (config.flags & 0x01) midi_sender->sendCC(...);
   if (config.flags & 0x02) osc_queue.enqueue(...);
   ```

### Configuration

1. **Utiliser `ComponentConfig` pour les paramètres**
2. **Valider les pins dans le Validator**
3. **NVS est géré automatiquement par `ConfigLoader`**

## 🎯 Checklist

### Composant simple
- [ ] Ajouter `ComponentType::XXX` dans `ComponentTypes.h`
- [ ] Créer `components/input/XxxDef.h` ou `components/output/XxxDef.h`
- [ ] Créer `processors/XxxProcessor.h/cpp`
- [ ] Ajouter dans `ProcessorRegistry::processComponent()`
- [ ] Ajouter dans `ComponentRegistry::init()`
- [ ] Ajouter dans `ValidationRegistry::init()`
- [ ] Mettre à jour le script `nidmi.sh` si nouveau dossier
- [ ] Tester compilation
- [ ] Tester avec moniteur MIDI/OSC

### Composant complexe
- [ ] Tout ce qui précède, plus :
- [ ] Créer `managers/XxxManager.h/cpp`
- [ ] Créer `managers/XxxValidator.h/cpp`
- [ ] Intégrer dans l'API si nécessaire

## 📝 TODO - Prochaines étapes

### Refactoring architecture family (en cours)

L'architecture actuelle utilise encore `input/` et `output/` comme dossiers. La migration vers l'architecture `family` est en cours.

#### Backend

- [ ] Ajouter `ComponentFamily` enum dans `ComponentDefinition.h`
- [ ] Ajouter `family` et `familyName` dans `ComponentDefinition`
- [ ] Retirer le système `variants` (remplacé par composants séparés)
- [ ] Créer le dossier `basic/` (déplacer depuis `input/` et `output/`)
- [ ] Créer le dossier `multiplexer/` avec `MuxDef.h`
  - Structure avec `MuxBase` + `HC4067` + `HC4051` (héritage)
  - Chaque modèle a son propre `id` (`hc4067`, `hc4051`)
- [ ] Mettre à jour `ComponentRegistry.cpp` pour charger depuis les nouveaux dossiers
- [ ] Mettre à jour le script `nidmi.sh` pour synchroniser les nouveaux dossiers

#### Frontend

- [ ] Ajouter le menu "Famille" dans l'UI
- [ ] Filtrer le menu "Composant" selon la famille sélectionnée
- [ ] Retirer la logique `id:variant` (ex: `mux:HC4067`)
- [ ] Utiliser `family` et `familyName` depuis l'API `/api/components/definitions`

#### API

- [ ] Ajouter `family` et `familyName` dans la réponse JSON de `/api/components/definitions`
- [ ] Optionnel : endpoint `/api/components/families` pour lister les familles

### Structure MUX avec héritage

```cpp
// src/components/multiplexer/MuxDef.h
namespace Components::Multiplexer {

struct MuxBase {
    // Code commun à tous les MUX
    static constexpr uint8_t NUM_ADDRESS_PINS = 4;
    static constexpr PinType PIN_TYPE = PinType::PIN_ANALOG;
    // Note: IS_COMPLEX supprimé - définir additionalPinCount > 0 dans createDefinition()
    // ...
};

struct HC4067 : MuxBase {
    static constexpr const char* ID = "hc4067";
    static constexpr const char* DISPLAY_NAME = "HC4067 (16 canaux)";
    static constexpr uint8_t NUM_CHANNELS = 16;
    static constexpr bool IMPLEMENTED = true;
    
    static ComponentDefinition createDefinition();
};

struct HC4051 : MuxBase {
    static constexpr const char* ID = "hc4051";
    static constexpr const char* DISPLAY_NAME = "HC4051 (8 canaux)";
    static constexpr uint8_t NUM_CHANNELS = 8;
    static constexpr bool IMPLEMENTED = false;  // Pas encore implémenté
    
    static ComponentDefinition createDefinition();
};

} // namespace
```

---

*Guide mis à jour pour l'architecture v3.0 avec ComponentFamily*
