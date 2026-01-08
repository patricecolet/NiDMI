# Guide d'Implémentation de Nouveaux Composants

Ce guide s'adresse aux stagiaires et développeurs qui souhaitent ajouter de nouveaux types de composants à NiDMI (ex: encodeurs rotatifs, capteurs touch, etc.).

## 📋 Table des matières

1. [Architecture des composants](#architecture-des-composants)
2. [Création d'un nouveau composant](#création-dun-nouveau-composant)
3. [Intégration dans ComponentManager](#intégration-dans-componentmanager)
4. [Ajout à l'interface web](#ajout-à-linterface-web)
5. [Bonnes pratiques](#bonnes-pratiques)

## 🏗️ Architecture des composants

### Structure actuelle

```
src/
├── components/
│   ├── Button.h           # Bouton poussoir (Note On/Off)
│   ├── Potentiometer.h    # Potentiomètre (CC MIDI)
│   ├── Led.h              # LED (réception MIDI)
│   └── AnalogMux.h        # Multiplexeur HC4067
├── ComponentManager.h/cpp # Gestionnaire central des composants
└── PinMapper.h/cpp        # Mapping pins ESP32
```

### Composants existants (exemples)

#### `Button.h` - Bouton poussoir
- **Fonctionnalité** : Détecte appui/relâchement → Note On/Off MIDI
- **Caractéristiques** :
  - Anti-rebond temporel (défaut 25ms)
  - INPUT_PULLUP (actif à LOW)
  - Note MIDI configurable

#### `Potentiometer.h` - Potentiomètre
- **Fonctionnalité** : Lecture analogique → CC MIDI
- **Caractéristiques** :
  - Filtre passe-bas avec hystérésis (réduit jitter)
  - Mapping 0-4095 → 0-127
  - CC MIDI configurable

#### `AnalogMux.h` - Multiplexeur HC4067
- **Fonctionnalité** : 16 canaux analogiques via 1 pin
- **Caractéristiques** :
  - 4 pins de sélection (S0-S3)
  - Pin EN optionnelle (active LOW)
  - Discard first reading pour stabilité

### Types de composants supportés

```cpp
enum class ComponentType : uint8_t {
    POTENTIOMETER = 0,  // Potentiomètre analogique
    BUTTON = 1,         // Bouton poussoir
    LED = 2             // LED (réception MIDI)
    // Ajouter votre nouveau type ici
};
```

## 🔧 Création d'un nouveau composant

### Étape 1 : Définir le composant dans `components/`

Créer un fichier header `src/components/VotreComposant.h` :

```cpp
#pragma once

#include <Arduino.h>
#include "../midi/MidiSender.h"

/**
 * @brief Description courte du composant
 * 
 * Exemple: EncoderRotary(D2, D3, 7, 1, router) enverra CC 7 sur canal 1.
 */
class EncoderRotary {
public:
    /**
     * @param pinA    Pin A de l'encodeur
     * @param pinB    Pin B de l'encodeur
     * @param cc      Numéro de CC MIDI (0-127)
     * @param channel Canal MIDI (1-16)
     * @param out     Routeur MIDI/OSC
     */
    EncoderRotary(uint8_t pinA, uint8_t pinB, uint8_t cc, uint8_t channel, MidiSender& out)
        : pinA(pinA), pinB(pinB), cc(cc), channel(channel), out(out), 
          position(0), lastA(false) {}

    /**
     * @brief Initialiser les pins
     */
    void begin() {
        pinMode(pinA, INPUT_PULLUP);
        pinMode(pinB, INPUT_PULLUP);
        lastA = digitalRead(pinA);
    }

    /**
     * @brief Mettre à jour l'état (appelé dans loop())
     */
    void update() {
        bool currentA = digitalRead(pinA);
        bool currentB = digitalRead(pinB);
        
        // Détecter changement (pattern quadrature)
        if (currentA != lastA) {
            if (currentA == currentB) {
                position++;
            } else {
                position--;
            }
            // Envoyer CC MIDI (clamp 0-127)
            uint8_t value = constrain(position, 0, 127);
            out.sendControlChange(channel, cc, value);
        }
        lastA = currentA;
    }

private:
    uint8_t pinA, pinB;
    uint8_t cc, channel;
    MidiSender& out;
    int16_t position;  // Position relative
    bool lastA;        // État précédent pinA
};
```

### Étape 2 : Ajouter le type dans `ComponentManager.h`

```cpp
// Types de composants supportés
enum class ComponentType : uint8_t {
    POTENTIOMETER = 0,
    BUTTON = 1,
    LED = 2,
    ENCODER = 3  // ← Nouveau type
};
```

### Étape 3 : Ajouter la gestion dans `ComponentManager.cpp`

#### 3.1 Dans `update()` - Ajouter le traitement

```cpp
void ComponentManager::update() {
    for (uint8_t i = 0; i < component_count; i++) {
        ComponentState& state = states[i];
        ComponentConfig& config = configs[i];
        
        switch (config.type) {
            case ComponentType::POTENTIOMETER:
                processPotentiometer(i);
                break;
            case ComponentType::BUTTON:
                processButton(i);
                break;
            case ComponentType::LED:
                // LED est traité par réception MIDI
                break;
            case ComponentType::ENCODER:  // ← Nouveau
                processEncoder(i);
                break;
        }
    }
}
```

#### 3.2 Créer la fonction `processEncoder()`

```cpp
void ComponentManager::processEncoder(uint8_t index) {
    ComponentState& state = states[index];
    ComponentConfig& config = configs[index];
    
    // Votre logique ici
    // Exemple: lire l'encodeur et envoyer CC MIDI
    // Note: Les pins sont dans config.gpio (ou deux pins séparées selon votre design)
    
    // Envoyer CC MIDI si changement détecté
    if (/* changement détecté */) {
        midi_sender->sendControlChange(
            config.midi_channel, 
            config.midi_param, 
            /* valeur */
        );
    }
    
    // OSC si activé
    if (config.flags & 0x02) {  // Flag OSC activé
        String oscAddress = (config.osc_address[0] != '\0') ? 
            String(config.osc_address) : "/encoder";
        // Envoyer OSC...
    }
}
```

#### 3.3 Dans `addComponent()` - Validation des pins

```cpp
bool ComponentManager::addComponent(uint8_t gpio, ComponentType type, 
                                    uint8_t midi_param, uint8_t channel, 
                                    MidiMessageType msg_type) {
    // ... validations existantes ...
    
    // Validation spécifique pour ENCODER
    if (type == ComponentType::ENCODER) {
        // Vérifier que les pins sont digitales et disponibles
        if (!pinMapper->isDigitalPin(gpio)) {
            Serial.printf("[ComponentManager] ERROR: GPIO %d is not digital\n", gpio);
            return false;
        }
        // Si vous avez besoin de 2 pins, ajoutez une validation supplémentaire
    }
    
    // ... reste du code ...
}
```

#### 3.4 Dans `loadFromNVS()` - Mapping rôle → type

```cpp
ComponentType type = ComponentType::POTENTIOMETER;
if (role == "Bouton") type = ComponentType::BUTTON;
else if (role == "LED") type = ComponentType::LED;
else if (role == "Encodeur") type = ComponentType::ENCODER;  // ← Ajouter
```

### Étape 4 : Ajouter à l'interface web

#### 4.1 Dans `web/js/components.js` - Ajouter le rôle

```javascript
function showRoleCards() {
    // ... code existant ...
    
    // Ajouter "Encodeur" dans les rôles disponibles
    // (selon le type de pin : digital ou analogique)
}
```

#### 4.2 Dans `web/index.html` - UI optionnelle

Si votre composant nécessite des paramètres supplémentaires, ajoutez-les dans le formulaire de configuration.

## 📐 Exemples concrets

### Exemple 1 : Encodeur rotatif (2 pins digitales)

```cpp
class EncoderRotary {
    // Voir exemple ci-dessus
};
```

**Points importants** :
- Détection quadrature (A/B)
- Anti-rebond (utiliser `Debounce` comme `Button`)
- Accumulation de position (incrément/décrément)

### Exemple 2 : Capteur touch ESP32-S3 (1 pin)

```cpp
#include <driver/touch_sensor.h>

class TouchSensor {
public:
    TouchSensor(uint8_t pin, uint8_t note, uint8_t channel, MidiSender& out)
        : touchPin(pin), note(note), channel(channel), out(out), 
          threshold(50), touched(false) {}

    void begin() {
        touchAttachInterrupt(touchPin, [](){}, threshold);
    }

    void update() {
        uint16_t value = touchRead(touchPin);
        bool isTouching = value < threshold;
        
        if (isTouching != touched) {
            touched = isTouching;
            if (touched) {
                out.sendNoteOn(channel, note, 127);
            } else {
                out.sendNoteOff(channel, note, 0);
            }
        }
    }

private:
    uint8_t touchPin;
    uint8_t note, channel;
    MidiSender& out;
    uint16_t threshold;
    bool touched;
};
```

## ✅ Bonnes pratiques

### Performance temps réel

1. **Pas de délais dans `update()`**
   ```cpp
   // ❌ MAUVAIS
   void update() {
       delay(10);  // Bloque le système
   }
   
   // ✅ BON
   void update() {
       static uint32_t lastTime = 0;
       uint32_t now = millis();
       if (now - lastTime < 10) return;  // Rate limiting
       lastTime = now;
       // ...
   }
   ```

2. **Utiliser des filtres pour analogique**
   ```cpp
   // Filtre passe-bas (déjà présent dans Potentiometer)
   float filtered = alpha * raw + (1.0f - alpha) * filtered;
   ```

3. **Anti-rebond pour digital**
   ```cpp
   // Utiliser la classe Debounce (comme Button)
   Debounce deb(25);  // 25ms de délai
   bool stable = deb.process(digitalRead(pin));
   ```

### Gestion mémoire

1. **Pas de `malloc/free` dans `loop()`**
   - Allouer au setup ou statiquement

2. **Utiliser des buffers fixes**
   ```cpp
   uint16_t buffer[16];  // ✅ Fixe
   // Pas: uint16_t* buffer = malloc(16 * sizeof(uint16_t));  // ❌
   ```

### Messages MIDI/OSC

1. **Utiliser `MidiSender`** pour l'envoi
   ```cpp
   midi_sender->sendControlChange(channel, cc, value);
   midi_sender->sendNoteOn(channel, note, velocity);
   ```

2. **Support OSC optionnel**
   ```cpp
   if (config.flags & 0x02) {  // Flag OSC activé
       osc_queue.enqueueFloat("/address", value);
   }
   ```

### Configuration

1. **Utiliser `ComponentConfig`** pour les paramètres
   ```cpp
   ComponentConfig config;
   config.gpio = pin;
   config.type = ComponentType::ENCODER;
   config.midi_param = 7;
   config.midi_channel = 1;
   ```

2. **Sauvegarder dans NVS** automatiquement
   - `ComponentManager` gère déjà la sauvegarde
   - Ajouter le mapping rôle → type dans `loadFromNVS()`

## 🐛 Débogage

### Serial logs

```cpp
Serial.printf("[VotreComposant] GPIO %d, valeur: %d\n", pin, value);
```

### Vérifications

1. **Pin correctement configurée** (`pinMode`)
2. **Valeurs dans les bonnes plages** (0-127 pour MIDI, 0-4095 pour analogique)
3. **Messages MIDI envoyés** (vérifier avec un moniteur MIDI)
4. **NVS sauvegardé** (redémarrer l'ESP32 et vérifier que la config persiste)

## 📚 Références

- `src/components/Button.h` : Exemple simple (digital + anti-rebond)
- `src/components/Potentiometer.h` : Exemple analogique (filtre + hystérésis)
- `src/components/AnalogMux.h` : Exemple complexe (multiplexeur)
- `src/ComponentManager.cpp` : Intégration complète

## 🎯 Checklist d'implémentation

- [ ] Créer `src/components/VotreComposant.h`
- [ ] Ajouter `ComponentType::VOTRE_TYPE` dans `ComponentManager.h`
- [ ] Ajouter `case` dans `ComponentManager::update()`
- [ ] Créer `processVotreComposant()` dans `ComponentManager.cpp`
- [ ] Ajouter validation dans `addComponent()`
- [ ] Ajouter mapping rôle → type dans `loadFromNVS()`
- [ ] Ajouter le rôle dans l'interface web (`components.js`)
- [ ] Tester avec un moniteur MIDI/OSC
- [ ] Vérifier sauvegarde NVS (redémarrage)
- [ ] Documenter les paramètres spécifiques

---

*Guide créé pour faciliter l'ajout de nouveaux composants*
*Basé sur l'architecture existante de NiDMI*
