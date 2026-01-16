# Guide d'Implémentation de Nouveaux Composants

Ce guide s'adresse aux stagiaires et développeurs qui souhaitent ajouter de nouveaux types de composants à NiDMI (ex: encodeurs rotatifs, capteurs touch, etc.).

## 📋 Table des matières

1. [Architecture des composants](#architecture-des-composants)
2. [Composants simples vs complexes](#composants-simples-vs-complexes)
3. [Création d'un composant simple](#création-dun-composant-simple)
4. [Création d'un composant complexe](#création-dun-composant-complexe)
5. [Intégration dans l'UI](#intégration-dans-lui)
6. [Bonnes pratiques](#bonnes-pratiques)

## 🏗️ Architecture des composants

### Structure des fichiers

```
src/
├── components/                    # Définitions des composants
│   ├── ComponentTypes.h           # Types de base (ComponentType, ComponentConfig, ComponentState)
│   ├── ComponentDefinition.h      # Structure de définition pour l'UI
│   ├── ComponentRegistry.h/cpp    # Registre central des composants
│   ├── ValidationRegistry.h/cpp   # Registre des validators
│   ├── input/                     # Composants d'entrée
│   │   ├── PotentiometerDef.h     # Définition potentiomètre
│   │   ├── ButtonDef.h            # Définition bouton
│   │   └── MuxDef.h/cpp           # Définition multiplexeur
│   └── output/                    # Composants de sortie
│       └── LedDef.h               # Définition LED
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

### Types de composants

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
    const char* id;              // "potentiometer", "button", etc.
    const char* displayName;     // "Potentiomètre", "Bouton", etc.
    const char* icon;            // Icône (optionnel)
    ComponentType type;          // Type enum
    PinType pinType;             // Type de pin requis
    bool implemented;            // true = disponible, false = grisé
    bool isComplex;              // true = nécessite un manager
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
    static constexpr bool IS_COMPLEX = false;
    
    // Valeurs par défaut
    static constexpr uint8_t DEFAULT_CC = 1;
    static constexpr uint8_t DEFAULT_CHANNEL = 1;
    
    // Validation inline
    static bool validate(uint8_t gpio) {
        return gpio < 48;  // N'importe quel GPIO valide
    }
    
    // Créer la définition
    static ComponentDefinition createDefinition() {
        return {
            ID, DISPLAY_NAME, nullptr,
            TYPE, PIN_TYPE, IMPLEMENTED, IS_COMPLEX
        };
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
    static constexpr bool IS_COMPLEX = true;  // ← Indique un composant complexe
    
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
    {"id":"potentiometer","displayName":"Potentiomètre","pinType":0,"implemented":true,"isComplex":false},
    {"id":"button","displayName":"Bouton","pinType":1,"implemented":true,"isComplex":false},
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

---

*Guide mis à jour pour l'architecture v2.0 avec ComponentRegistry*
