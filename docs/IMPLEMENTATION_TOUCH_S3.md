# 🎯 Implémentation complète des Touch Pins ESP32‑S3 dans NiDMI

## 📋 Contexte

L’ESP32‑S3 dispose de **touch sensors matériels** qui permettent de détecter des variations de capacité sur certaines pins (D0…D9 sur le XIAO ESP32‑S3).  
NiDMI possède déjà une bonne partie de l’infrastructure pour exploiter ces pins, mais l’implémentation doit être **branchée de bout en bout** :

- description des pins S3 (`touch: true`),
- composant NiDMI `Touch`,
- `TouchProcessor` (lecture + logique MIDI/OSC),
- intégration API/OSC/MIDI,
- interface web pour configurer les Touch.

Ce document décrit **comment tout ça s’enchaîne réellement dans le repo actuel**, et ce qu’il reste à vérifier/compléter.

---

## 1. Vue d’ensemble du pipeline Touch S3

1. **Hardware / description des pins S3**
   - Fichier : `src/hardware/pincaps_s3.cpp`
   - But : marquer quelles pins supportent le touch (`"touch": true`).
2. **Mapping vers la config NiDMI**
   - Fichiers : `src/utils/PinMapper.*`, `src/config/SystemConfig.*`
   - But : exposer ces capacités (dont `touch`) au système de config / à l’UI.
3. **Composant NiDMI de type Touch**
   - Fichiers : `src/components/basic/TouchDef.*`, `src/components/ComponentTypes.h`, `src/components/Definitions.h`, `src/components/MidiMessageFactory.h`, `src/components/FormFieldHelpers.h`.
4. **Processor Touch (backend temps réel)**
   - Fichiers : `src/processors/TouchProcessor.*`, `src/processors/Processors.h`, `src/processors/ProcessorRegistry.*`, `src/managers/ComponentManager.*`.
5. **API / OSC / MIDI**
   - Fichiers : `src/NiDMI.cpp`, `src/api/*`, `src/osc/*`, `src/midi/*`, `src/utils/*`.
6. **Interface web**
   - Fichiers : `web/index.html` / `build/index.min.html`, `web/js/*` (définition des types de composants, formulaires, sauvegarde de config).

---

## 2. Niveau hardware : Touch sur ESP32‑S3

### 2.1 Description des pins S3

- Fichier : `src/hardware/pincaps_s3.cpp`
- Contenu principal :

```12:23:src/hardware/pincaps_s3.cpp
const char PINCAPS_S3[] PROGMEM = R"JSON({
  "board":"xiao-esp32s3",
  "pins":[
    {"gpio":1,  "label":"D0", "caps":{"in":true,"out":true,"adc":true,"pwm":true,"touch":true},  "sensitive":false},
    {"gpio":2,  "label":"D1", "caps":{"in":true,"out":true,"adc":true,"pwm":true,"touch":true},  "sensitive":false},
    ...
    {"gpio":10, "label":"D9", "caps":{"in":true,"out":true,"adc":false,"pwm":true,"touch":true}, "sensitive":false}
  ],
  "bus":{
    "i2c":{"sda":4,"scl":5},
    "spi":{"mosi":7,"miso":6,"sck":8},
    "uart":{"tx":43,"rx":44}
  }
})JSON";
```

- **À vérifier** :
  - `touch:true` n’est présent que sur les pins réellement touch‑capable.
  - `NIDMI_NO_S3` **n’est pas** défini quand on veut supporter S3.

### 2.2 Macros de compilation Touch

- Fichier : `src/processors/TouchProcessor.cpp`
- La disponibilité du touch est déterminée par des macros (IDF/Arduino + `CONFIG_SOC_TOUCH_SENSOR_SUPPORTED`).
- **Action** : s’assurer que la toolchain S3 (Arduino/IDF) active bien `CONFIG_SOC_TOUCH_SENSOR_SUPPORTED`. Sinon `TOUCH_AVAILABLE` restera à `0` et le backend Touch sera désactivé.

---

## 3. Composant NiDMI `Touch`

### 3.1 Enregistrement du composant

- Fichier : `src/components/basic/TouchDef.cpp`

```1:7:src/components/basic/TouchDef.cpp
#include "TouchDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_touch = ComponentRegistry::registerDefinition(
    Components::Touch::createDefinition()
);
```

- **Effet** : dès que ce module est linké, un type de composant `Touch` apparaît dans le `ComponentRegistry`.

### 3.2 Types et définitions

À vérifier/compléter dans :

- `src/components/ComponentTypes.h`  
  - Présence d’un `ComponentType` pour le Touch (nom à aligner avec ce qui est utilisé dans `TouchProcessor` / defs).
- `src/components/Definitions.h`  
  - Définition structurée du composant : nom, catégorie, champs de config (gpio, midi_channel, midi_param, paramètres de seuil…).
- `src/components/MidiMessageFactory.h`  
  - Mappage entre type de composant Touch et messages MIDI (Note On/Off, CC, etc.).
- `src/components/FormFieldHelpers.h`  
  - Champs visibles dans l’UI (seuil, mode, vélocité, aftertouch…).

**Objectif** : à partir d’une entrée JSON, le backend sait créer un `ComponentConfig` pour un Touch avec :

- `gpio` (pin S3 à utiliser),
- `midi_channel`, `midi_param` (note ou CC),
- paramètres optionnels :
  - `potMin` (seuil principal),
  - `customInt1` (seuil velocity),
  - `customInt2` (seuil aftertouch).

---

## 4. TouchProcessor : cœur du backend Touch

### 4.1 Fichiers

- `src/processors/TouchProcessor.cpp`
- `src/processors/TouchProcessor.h`
- `src/processors/Processors.h`
- `src/processors/ProcessorRegistry.*`
- `src/managers/ComponentManager.*`

### 4.2 Lecture et filtrage des valeurs

Le `TouchProcessor` :

- lit périodiquement la valeur brute via `touchRead(gpio)`,
- fait une moyenne glissante sur plusieurs échantillons,
- enregistre des stats par pin (min/max, logs),
- calcule une **baseline**.

La baseline est stockée par GPIO, après un certain nombre de mesures (extrait simplifié) :

```44:69:src/processors/TouchProcessor.cpp
static uint16_t readTouchValue(uint8_t gpio) {
    const int samples = 5;
    uint32_t sum = 0;
    static uint16_t last_raw_reads[49][5] = {0};
    static uint8_t sample_idx[49] = {0};

    for (int i = 0; i < samples; i++) {
        uint16_t raw = touchRead(gpio);
        sum += raw;
        last_raw_reads[gpio][sample_idx[gpio]] = raw;
        sample_idx[gpio] = (sample_idx[gpio] + 1) % 5;
        delayMicroseconds(200);
    }
    uint16_t avg = sum / samples;
    ...
    return avg;
}
```

```72:121:src/processors/TouchProcessor.cpp
static bool establishBaseline(uint8_t gpio, uint32_t& baseline) {
    static uint32_t baseline_value[49] = {0};
    static uint32_t baseline_sum[49] = {0};
    static uint8_t baseline_count[49] = {0};
    static bool baseline_set[49] = {false};
    static uint16_t baseline_min[49] = {65535};
    static uint16_t baseline_max[49] = {0};
    ...
    uint16_t touch_value = readTouchValue(gpio);
    baseline_sum[idx] += touch_value;
    baseline_count[idx]++;
    ...
    if (baseline_count[idx] >= 20) {
        baseline_value[idx] = baseline_sum[idx] / baseline_count[idx];
        baseline_set[idx] = true;
        baseline = baseline_value[idx];
        ...
        return true;
    }
    return false;
}
```

### 4.3 Calcul des seuils

Les seuils sont dérivés de la configuration du composant :

```123:155:src/processors/TouchProcessor.cpp
static void calculateThresholds(
    const ComponentConfig& config,
    uint32_t baseline,
    uint32_t& touch_threshold,
    uint32_t& velocity_threshold,
    uint8_t& aftertouch_threshold
) {
    // Touch threshold : potMin si configuré, sinon 80% de baseline
    ...
    // Velocity threshold : customInt1 si configuré, sinon touch_threshold
    ...
    // Aftertouch threshold : customInt2 si configuré, sinon 4
    ...
}
```

En fonction du **mode de sortie** (NOTE, CC, aftertouch, etc.), le fichier implémente ensuite les fonctions qui :

- déterminent **quand envoyer un Note On**,
- calculent la **velocity**,
- déterminent **quand envoyer un Note Off**,
- gèrent éventuellement un **aftertouch**.

### 4.4 Intégration dans la boucle de traitement

À vérifier :

- `TouchProcessor` est enregistré dans `ProcessorRegistry` comme processor associé au type de composant Touch.
- `Processors.h` et `ComponentManager` appellent bien le traitement Touch pour les composants de type Touch.

**Résultat attendu** : dès qu’une config JSON contient un composant Touch, le `TouchProcessor` est automatiquement instancié et exécuté à chaque tick.

---

## 5. Intégration API / OSC / MIDI

Fichiers à considérer :

- `src/NiDMI.cpp`
- `src/api/ComponentsAPI.cpp`
- `src/api/PinAPI.cpp`
- `src/api/SystemAPI.cpp`
- `src/api/OSC_API.cpp`
- `src/api/UsbMidiAPI.*`, `src/api/RTPAPI.*`, `src/api/CacheAPI.cpp`, `src/api/APICommon.h`
- `src/osc/OSCManager.cpp`, `src/osc/OSCQueue.*`, `src/osc/OSCConfigLoader.*`, `src/osc/OSCCalibrationHandler.*`
- `src/midi/MidiRouter.*`, `src/midi/MidiMessageType.cpp`, `src/midi/handlers/*`
- `src/utils/ComponentInitializer.*`, `src/utils/JSONParser.*`, `src/utils/PinMapper.*`

**Points de contrôle** :

- Les endpoints qui retournent :
  - la **liste des composants** doivent inclure les Touch (avec leurs paramètres),
  - la **liste des pins** doit refléter les capacités `touch:true` sur les pins S3.
- Les événements générés par `TouchProcessor` :
  - sont routés correctement vers MIDI/OSC via `MidiRouter` / `OSCManager`,
  - respectent les types de messages attendus (Note, CC, etc.).

---

## 6. Interface web : configurer un Touch

Objectifs côté UI :

- Pouvoir créer/éditer un composant de type **Touch**.
- Restreindre les pins proposées aux pins S3 marquées `touch:true` (optionnel mais recommandé).
- Exposer les paramètres essentiels du Touch (`potMin`, `customInt1`, `customInt2`…).

Fichiers typiques :

- `web/js/definitions.js` (définition des types de composants côté front).
- `web/js/component-form.js`, `web/js/form-generator.js` (construction des formulaires).
- `web/js/component-config.js`, `web/js/component-helpers.js` (logique autour des composants).
- `web/index.html` / `build/index.min.html` (inclusion des scripts, éventuels templates).

**Travail à faire / vérifier** :

- Ajout d’un type `Touch` dans la liste des types de composants utilisables.
- Mapping des champs UI ↔ champs JSON ↔ `ComponentConfig` consommé par `TouchProcessor`.
- Sauvegarde / rechargement d’une config contenant des Touch sans erreur.

---

## 7. Plan d’implémentation par étapes

### Étape 1 – Backend Touch fonctionnel

1. **Compiler pour ESP32‑S3** et vérifier que `TOUCH_AVAILABLE` vaut 1 (macro configurée correctement).
2. Vérifier/compléter :
   - `ComponentType` Touch dans `ComponentTypes.h` + wires dans `Definitions.h`.
   - Enregistrement `TouchDef` (`TouchDef.cpp`) et `TouchProcessor` (`ProcessorRegistry`).
3. Avec une config JSON de test contenant un Touch :
   - vérifier que les logs `TOUCH_INFO` / `TOUCH_WARN` apparaissent,
   - vérifier que `TouchProcessor` lit bien les valeurs (`touchRead`) et établit une baseline.

### Étape 2 – Intégration API

1. Vérifier via l’API que :
   - les composants Touch apparaissent dans les réponses de `ComponentsAPI`,
   - les pins S3 ont bien la capacité `touch` dans les réponses de `PinAPI` / `SystemAPI`.
2. Tester sauvegarde / reload d’un preset contenant des Touch.

### Étape 3 – Interface web

1. Ajouter le type `Touch` côté front (définition + formulaire).
2. Relier les champs de l’UI aux paramètres utilisés par `TouchProcessor` (`potMin`, `customInt1`, `customInt2`).
3. Vérifier que l’UI :
   - propose les pins touch‑capable du S3,
   - sauvegarde/recharge les configs Touch sans erreur.

### Étape 4 – Tests matériels

1. **Test simple Note On/Off** :
   - Créer un composant Touch sur une pin S3,
   - toucher / relâcher le pad,
   - vérifier les logs série et la sortie MIDI (Note On/Off).
2. **Test des seuils** :
   - ajuster `potMin`, `customInt1`, `customInt2`,
   - observer l’impact sur la sensibilité, la vélocité, l’aftertouch.
3. **Test charge / robustesse** :
   - plusieurs Touch actifs en parallèle,
   - vérifier qu’il n’y a ni WDT ni saturation OSC/MIDI (cf. `docs/OPTIMISATION_TOUCH_S3.md` pour les optimisations possibles).

---

## 8. Résumé (checklist rapide)

- **Hardware** : `pincaps_s3.cpp` marque correctement les pins `touch:true` et `TOUCH_AVAILABLE` est actif sur S3.
- **Composants** : type Touch défini (`ComponentTypes`, `Definitions`, `MidiMessageFactory`, `FormFieldHelpers`, `TouchDef`).
- **Processor** : `TouchProcessor` lit les pins, calcule baseline + seuils, et est bien appelé via `ProcessorRegistry` / `ComponentManager`.
- **API / OSC / MIDI** : les composants Touch et les pins touch‑capable sont visibles et leurs événements sont routés correctement.
- **UI** : type Touch configurable dans le front, sauvegarde/reload OK.
- **Tests** : Note On/Off, réglage des seuils, tests de charge sans WDT ni surcharge réseau.

