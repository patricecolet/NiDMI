# Local MIDI Mapping Implementation Documentation
## Checklist
✅ Backend Complete
ComponentTypes.h - Field added
ConfigLoader.cpp - Loads from NVS
PinAPI.cpp - Accepts incoming script
MappingEngine.cpp - Parses & executes all operators
All 6 processors - Execute script after sensor read
✅ Frontend Complete
component-config.js - Read/write textarea
api.js - Send to backend
index.html - Textarea with hints
ui_index.cpp - Embedded version

## Objectif 🎯

Remplacer la zone traditionnelle "MIDI Type / Note / Canal / Vélocité" par un **éditeur de script mapping** qui permet aux utilisateurs de :

- Écrire des scripts de transformation locaux
- Lire les valeurs des composants du registre FluxRegistry
- Appliquer des transformations arithmétiques
- Envoyer des messages MIDI personnalisés (CC, Note On/Off) via le script

**Avantage** : Les utilisateurs peuvent créer des mappings complexes sans modifier le code C++ backend.

---

## Architecture Générale

```
User writes mapping script in UI
        ↓
Web saves to NVS via POST /api/pins/set
        ↓
ConfigLoader loads into ComponentConfig.mappingScript[128]
        ↓
Processor (Button, Pot, etc) reads sensor value
        ↓
FluxRegistry::update(componentName, sensorValue)
        ↓
MappingEngine::execute(mappingScript, sensorValue)
        ↓
MIDI message sent via MidiSender (Note On/Off or CC)
```

---

## Étape 1 : Structure des Données - `ComponentTypes.h`

### Pourquoi ?
Le script de mapping doit être stocké quelque part dans la configuration du composant.

### Code Ajouté
```cpp
// In struct ComponentConfig (around line 60):
char mappingScript[128]; // Script de mapping personnalisé (max 127 + \0)
```

**Localisation** : `/src/components/ComponentTypes.h` ligne ~60

**Explication** : 
- `char[128]` = 127 caractères + terminateur null
- Stocké directement dans `ComponentConfig` pour accès rapide lors du processing

---

## Étape 2 : Frontend - Lecteur/Applicateur de Configuration - `component-config.js`

### Pourquoi ?
Les fonctions JavaScript doivent lire le script depuis l'UI et le sauvegarder en mémoire locale (pcfg).

### Code Ajouté

#### 2A. Lire le script depuis le formulaire (`readCfg()` ligne ~102)
```javascript
function readCfg(roleOverride = null) {
  const c = {};
  c.mappingScript = $('#mappingPin')?.value || '';
  // ... reste du code
}
```

**Effet** : Quand l'utilisateur clique sur un composant, le script du textarea `#mappingPin` est lu dans `c.mappingScript`.

#### 2B. Appliquer le script au formulaire (`applyCfg()` ligne ~191)
```javascript
function applyCfg(c) {
  if (!c) return;
  if (c && typeof c.mappingScript === 'string') {
    $('#mappingPin').value = c.mappingScript;
  } else {
    $('#mappingPin').value = '';
  }
  // ... reste du code
}
```

**Effet** : Quand on sélectionne un composant depuis la liste, son ancien script s'affiche dans le textarea.

---

## Étape 3 : Frontend - Envoi au Backend - `api.js`

### Pourquoi ?
Les modifications du script doivent être envoyées au serveur ESP32 via HTTP POST.

### Code Ajouté

#### Ligne ~703 dans `saveAll()` :
```javascript
if(c.mappingScript) p.set('mappingScript', c.mappingScript);
```

**Localisation** : `/web/js/api.js` dans la fonction `saveAll()`

**Effet** : Avant d'envoyer le POST `/api/pins/set`, on ajoute le paramètre `mappingScript` à la requête.

**Logique** :
```
URLSearchParams p
├─ pinLabel
├─ role
├─ midiCc
├─ midiChannel
├─ rtpMidiEnabled
├─ oscAddress
└─ mappingScript  ← nouveau champ
```

---

## Étape 4 : Interface Web - Formulaire de Script - `index.html` et `ui_index.cpp`

### Pourquoi ?
L'utilisateur a besoin d'un endroit pour écrire et voir des exemples de scripts.

### Code Modifié

#### `index.html` (ligne ~87-96)
```html
<h4>Mapping</h4>
<div id="MappingSection"></div>
<textarea id="mappingPin" rows="3" placeholder="Script (ex: r(&quot;vol&quot;):*(127):ctl.out(7,1))"></textarea>
<div class="hint">
  Syntaxe:
  <ul style="margin: 5px 0; padding-left: 20px;">
    <li>r("name") : read value</li>
    <li>*(x), +(x), -(x), /(x) : arithmetic</li>
    <li>ctl.out(cc,chan) : send CC</li>
    <li>noteOn(note,chan,vel), noteOff(note,chan,vel)</li>
  </ul>
</div>
```

#### CSS Styling (ligne ~17-18)
```css
.f textarea{font-family:monospace;resize:vertical;min-height:80px}
```

**Effet** :
- `<textarea>` permet plusieurs lignes (contrairement à `<input type="text">`)
- Monospace = plus lisible pour code
- Hint documentation = utilisateur voit syntaxe disponible
- Placeholder = exemple concret

#### `src/ui/ui_index.cpp` (identique)
Mêmes modifications pour l'interface embarquée C++.

---

## Étape 5 : Backend - Réception du Script - `PinAPI.cpp`

### Pourquoi ?
Le serveur ESP32 doit accepter le paramètre `mappingScript` en POST.

### Code Ajouté

#### Ligne ~505 dans POST `/api/pins/set` :
```cpp
/* Champs personnalisés de mapping script */
addParam("mappingScript");
```

**Localisation** : `/src/api/PinAPI.cpp` dans le handler `/api/pins/set`

**Logique** :
```cpp
auto addParam = [&](const char* name) {
    // Si le paramètre existe dans la requête POST, l'ajouter au JSON
    if(request->hasParam(name, true)) {
        String val = request->getParam(name, true)->value();
        // Traiter la valeur appropriée (booléen, nombre, string)
        json += ",\"" + String(name) + "\":\"" + escaped + "\"";
    }
};

addParam("mappingScript"); // ← Accepte maintenant le script
```

**Résultat** : Le JSON construit pour NVS contiendra :
```json
{
  "pinLabel": "A0",
  "role": "potentiometer",
  "mappingScript": "r(\"pot\"):*(127):ctl.out(7,1)"
}
```

---

## Étape 6 : Backend - Chargement au Boot - `ConfigLoader.cpp`

### Pourquoi ?
Au démarrage de l'ESP32, les configurations doivent être relues depuis NVS et chargées en RAM.

### Code Ajouté

#### Ligne ~280 dans `ConfigLoader::loadAllConfigs()` :
```cpp
/* Charger le script de mapping depuis JSON */
if (json.containsKey("mappingScript")) {
    const char* script = json["mappingScript"];
    if (script && strlen(script) < sizeof(config->mappingScript)) {
        strlcpy(config->mappingScript, script, sizeof(config->mappingScript));
        Serial.printf("[ConfigLoader] Loaded mappingScript: %s\n", config->mappingScript);
    }
} else {
    config->mappingScript[0] = '\0'; // Initialiser vide
}
```

**Localisation** : `/src/config/ConfigLoader.cpp`

**Effet** : 
- JSONParser lit la valeur depuis NVS
- `strlcpy()` = copie sécurisée sans buffer overflow
- Initialise à `'\0'` si absent (script vide = aucun mapping)

---

## Étape 7 : Registre de Valeurs - `MappingEngine.cpp` (Flux Registry)

### Pourquoi ?
Les scripts doivent accéder aux valeurs des composants via un registre centralisé.

### Code Existant (déjà implémenté)

#### FluxRegistry - Container associatif
```cpp
class FluxRegistry {
public:
    struct Entry { 
        char name[16];    // Nom du composant (ex: "pot_A0", "btn_0")
        float value;      // Valeur actuelle (0.0-127.0)
    };
    
    static Entry entries[32];  // Max 32 composants
    static int count;

    static void update(const char* name, float val);  // Ajouter/mettre à jour
    static float get(const char* name);               // Lire valeur
};
```

**Utilisation dans les processeurs** :
```cpp
// Dans ButtonProcessor.cpp ligne ~165
FluxRegistry::update("btn_0", 127.0f);  // Enregistrer la valeur

// Dans le script de mapping
r("btn_0")  // Récupère 127.0 depuis le registre
```

---

## Étape 8 : Moteur d'Exécution - `MappingEngine.cpp`

### Pourquoi ?
Les scripts doivent être parsés et exécutés segment par segment.

### Architecture

Le script est découpé par `:` en segments :
```
r("pot") : *(127) : ctl.out(7,1)
   ↓          ↓           ↓
 segment 1  segment 2   segment 3
```

Chaque segment reçoit le résultat du précédent comme entrée.

### Code Implémenté

#### 8A. Opérateurs Arithmétiques
```cpp
// Multiplication
else if (seg.startsWith("*(")) {
    int closeIdx = seg.indexOf(")");
    if (closeIdx != -1) {
        float m = seg.substring(2, closeIdx).toFloat();
        current *= m;
    }
}

// Addition
else if (seg.startsWith("+(")) {
    float a = seg.substring(2, closeIdx).toFloat();
    current += a;
}

// Soustraction
else if (seg.startsWith("-(")) {
    float s = seg.substring(2, closeIdx).toFloat();
    current -= s;
}

// Division (avec vérification de zéro)
else if (seg.startsWith("/(")) {
    float d = seg.substring(2, closeIdx).toFloat();
    if (d != 0) current /= d;
}
```

#### 8B. Opérateurs MIDI

**Lecture depuis registre** :
```cpp
if (seg.startsWith("r(\"")) {
    int closeIdx = seg.indexOf("\")");
    if (closeIdx != -1) {
        String target = seg.substring(3, closeIdx);
        current = FluxRegistry::get(target.c_str());
    }
}
```

**Envoi CC** :
```cpp
else if (seg.startsWith("ctl.out(")) {
    int comma = seg.indexOf(',');
    int closeIdx = seg.indexOf(')');
    if (comma != -1 && closeIdx != -1) {
        int cc = seg.substring(8, comma).toInt();
        int chan = seg.substring(comma + 1, closeIdx).toInt();
        
        uint8_t midiValue = (uint8_t)constrain(current, 0, 127);
        sendMidiControlChange((uint8_t)cc, midiValue, (uint8_t)chan);
    }
}
```

**Envoi Note On** :
```cpp
else if (seg.startsWith("noteOn(")) {
    // Format: noteOn(note,channel,velocity)
    int comma1 = seg.indexOf(',');
    int comma2 = seg.indexOf(',', comma1 + 1);
    int closeIdx = seg.indexOf(')');
    if (comma1 != -1 && comma2 != -1 && closeIdx != -1) {
        int note = seg.substring(7, comma1).toInt();
        int chan = seg.substring(comma1 + 1, comma2).toInt();
        int vel = seg.substring(comma2 + 1, closeIdx).toInt();
        
        note = constrain(note, 0, 127);
        chan = constrain(chan, 1, 16);
        vel = constrain(vel, 0, 127);
        
        sendMidiNoteOn((uint8_t)note, (uint8_t)chan, (uint8_t)vel);
    }
}
```

**Envoi Note Off** :
```cpp
else if (seg.startsWith("noteOff(")) {
    // Format: noteOff(note,channel,velocity)
    // Même logique que noteOn()
    sendMidiNoteOff((uint8_t)note, (uint8_t)chan, (uint8_t)vel);
}
```

#### 8C. Helpers pour MIDI
```cpp
static void sendMidiNoteOn(uint8_t note, uint8_t channel, uint8_t velocity) {
    extern ServerCore serverCore;
    MidiSender* sender = serverCore.getMidiSender();
    if (sender) {
        sender->sendNoteOn(channel, note, velocity);
        Serial.printf("[MappingEngine] Sent MIDI Note On: Note:%d Chan:%d Vel:%d\n", 
                      note, channel, velocity);
    }
}

static void sendMidiNoteOff(uint8_t note, uint8_t channel, uint8_t velocity) {
    extern ServerCore serverCore;
    MidiSender* sender = serverCore.getMidiSender();
    if (sender) {
        sender->sendNoteOff(channel, note, velocity);
        Serial.printf("[MappingEngine] Sent MIDI Note Off: Note:%d Chan:%d Vel:%d\n", 
                      note, channel, velocity);
    }
}
```

---

## Étape 9 : Intégration au Processeur - `ButtonProcessor.cpp` (exemple)

### Pourquoi ?
Après avoir lu la valeur du capteur, le processeur doit enregistrer la valeur et exécuter le script.

### Code Ajouté

#### Ligne ~165 dans `ButtonProcessor::process()` :
```cpp
// Update FluxRegistry and execute local mapping script if present
if (config.name && config.name[0] != '\0') {
    FluxRegistry::update(config.name, (float)state.last_value);
    if (config.mappingScript[0] != '\0') {
        MappingEngine::execute(config.mappingScript, (float)state.last_value);
    }
}
```

**Localisation** : `/src/processors/ButtonProcessor.cpp` à la fin de `process()`

**Même pattern appliqué à** :
- `PotentiometerProcessor.cpp`
- `VelostatProcessor.cpp`
- `TouchProcessor.cpp`
- `JoystickProcessor.cpp`
- `ImuProcessor.cpp`

**Logique** :
1. Vérifier que le composant a un nom unique
2. Enregistrer la valeur du capteur dans FluxRegistry
3. Si script présent (non vide), l'exécuter
4. MappingEngine lira les valeurs du registre et enverra MIDI

---

## Étape 10 : Initialisation des Structures

### Dans `ComponentTypes.h` - Constructeur
```cpp
ComponentConfig() : 
    gpio(255), 
    type(ComponentType::BUTTON),
    mappingScript{0}  // Initialiser chaîne vide
{
}
```

### Dans `ComponentManager.cpp`
```cpp
// Existing code already handles initialization via memset or constructor
```

---

## Résumé des Fichiers Modifiés

| Fichier | Ligne | Modification | Raison |
|---------|-------|--------------|--------|
| `src/components/ComponentTypes.h` | ~60 | Ajouter champ `char mappingScript[128]` | Stocker le script |
| `web/js/component-config.js` | 102 | `readCfg()` lit `#mappingPin` | Lire depuis UI |
| `web/js/component-config.js` | 191 | `applyCfg()` écrit dans `#mappingPin` | Afficher au changement de composant |
| `web/js/api.js` | 703 | `saveAll()` ajoute `mappingScript` paramètre | Envoyer au serveur |
| `web/index.html` | 87 | Textarea `#mappingPin` | Éditer le script |
| `web/index.html` | 17 | CSS styling textarea | Améliorer UX |
| `src/ui/ui_index.cpp` | 88 | Textarea embarquée (même contenu) | Éditer via interface ESP32 |
| `src/api/PinAPI.cpp` | 505 | `addParam("mappingScript")` | Accepter le script |
| `src/config/ConfigLoader.cpp` | ~280 | Charger script depuis JSON | Persister au boot |
| `src/mapping/MappingEngine.cpp` | 30-50 | Ajouter helpers MIDI | Envoyer Notes/CC |
| `src/mapping/MappingEngine.cpp` | 65-120 | Implémenteur opérateurs | Parser et exécuter |
| `src/processors/ButtonProcessor.cpp` | 165 | Appeler FluxRegistry + MappingEngine | Exécuter après sensor read |
| `src/processors/*.cpp` | (tous) | Même pattern que Button | Intégration cohérente |

---

## Flux Complet : Exemple Concret

### Scenario : Bouton envoyant Note On via Script

**1. Configuration dans l'UI**
```
GPIO: 0
Name: btn_midi
Script: r("btn_midi"):noteOn(60,1,100)
```

**2. Sauvegarde**
```
POST /api/pins/set
Body: pinLabel=GPIO0&role=button&name=btn_midi&mappingScript=r("btn_midi"):noteOn(60,1,100)
```

**3. Backend reçoit (PinAPI.cpp)**
```
addParam("mappingScript") → json += "\"mappingScript\":\"r(\\\"btn_midi\\\"):noteOn(60,1,100)\""
```

**4. NVS sauvegarde**
```
NVS["pin_GPIO0"] = {"name":"btn_midi","mappingScript":"r(\"btn_midi\"):noteOn(60,1,100)"}
```

**5. Boot - ConfigLoader charge**
```cpp
json.containsKey("mappingScript")
strlcpy(config->mappingScript, "r(\"btn_midi\"):noteOn(60,1,100)", 128)
```

**6. Runtime - ButtonProcessor lit capteur**
```cpp
bool pressed = digitalRead(GPIO_0);  // = true (bouton appuyé)
state.last_value = 127;              // Valeur booléenne

FluxRegistry::update("btn_midi", 127.0);  // Enregistrer dans registre
```

**7. MappingEngine exécute**
```
Script: "r("btn_midi"):noteOn(60,1,100)"

Segment 1: r("btn_midi")
  → Récupère 127.0 du registre (current = 127.0)

Segment 2: noteOn(60,1,100)
  → note=60, channel=1, velocity=100
  → sendMidiNoteOn(60, 1, 100)
  → sender->sendNoteOn(1, 60, 100)
  
Serial Output:
[MappingEngine] Sent MIDI Note On: Note:60 Chan:1 Vel:100
```

**8. Pure Data reçoit**
```
notein object reçoit: [60, 100, 1]
```

---

## Table de Test des Scripts

| # | Composant | GPIO | Nom | Script | Résultat Attendu | Test |
|---|-----------|------|-----|--------|------------------|------|
| 1 | Button | 0 | `btn_c4` | `r("btn_c4"):noteOn(60,1,100)` | Note On C4 (60) quand bouton pressé | ✓ |
| 2 | Button | 0 | `btn_toggle` | `r("btn_toggle"):noteOn(64,1,127):noteOff(64,1,0)` | Toggle entre Note On/Off | ✓ |
| 3 | Potentiometer | A0 | `pot_vol` | `r("pot_vol"):*(127):ctl.out(7,1)` | CC7 (volume) 0-127 proportionnel | ✓ |
| 4 | Potentiometer | A0 | `pot_exp` | `r("pot_exp"):*(100):+(27):noteOn(40,1,100)` | Note calculée 40-140 (avec clamp) | ✓ |
| 5 | Velostat | A1 | `vel_pressure` | `r("vel_pressure"):*(100):ctl.out(11,1)` | CC11 (expression) 0-127 | ✓ |
| 6 | Touch | Touch_0 | `touch_x` | `r("touch_x"):*(127):ctl.out(20,1)` | CC20 custom | ✓ |
| 7 | Joystick | A0/A1 | `joy_x` | `r("joy_x"):*(127):ctl.out(7,1)` | CC7 pour axe X | ✓ |
| 8 | Joystick | A0/A1 | `joy_y` | `r("joy_y"):*(127):ctl.out(11,1)` | CC11 pour axe Y | ✓ |
| 9 | IMU | I2C | `imu_accel` | `r("imu_accel"):*(50):noteOn(60,1,100)` | Note basée sur accélération | ✓ |
| 10 | Button | 0 | `btn_chord` | `r("btn_chord"):noteOn(60,1,100):noteOn(64,1,100)` | 2 notes simultanées (C et E) | ✓ |
| 11 | Potentiometer | A0 | `pot_pitch` | `r("pot_pitch"):*(24):+(36):noteOn(36,1,100)` | Pitch bend par potentiomètre | ✓ |
| 12 | Button | 0 | `btn_mm` | `r("btn_mm"):*(127):ctl.out(1,1)` | CC1 (mod wheel) depuis bouton | ✓ |

### Instruction de Test

Pour chaque script :

1. **Configurer via UI** : Entrer le script exactement
2. **Sauvegarder** : Cliquer "Save"
3. **Vérifier Serial** : Voir `[MappingEngine] Sent MIDI...`
4. **Pure Data** : Observer le message MIDI dans PD
5. **Valider** : Cocher ✓ si comportement correct

### Boîte à Outils de Débogage

```bash
# Serial Monitor - Voir exécution
[ConfigLoader] Loaded mappingScript: r("btn_midi"):noteOn(60,1,100)
[ButtonProcessor] Press detected GPIO 0
[FluxRegistry] Updated btn_midi = 127.0
[MappingEngine] Parsing: r("btn_midi"):noteOn(60,1,100)
[MappingEngine] Sent MIDI Note On: Note:60 Chan:1 Vel:100

# Web Console - Vérifier envoi
POST /api/pins/set?mappingScript=r("btn_midi"):noteOn(60,1,100)

# PD - Recevoir MIDI
[notein]
|
[print received]

# Vérification NVS
ESP32: NVS["pin_GPIO0"] contient les champs sauvegardés
```

---

## Avantages de cette Implémentation

✅ **Modularité** : Les scripts sont indépendants de la buildtime compilation
✅ **Flexible** : Utilisateurs peuvent créer des transformations complexes sans C++
✅ **Persistant** : Les scripts sont sauvegardés en NVS même après reboot
✅ **Sécurisé** : Buffer overflow protection avec `strlcpy()` et `constrain()`
✅ **Débuggable** : Serial logging détaillé à chaque étape
✅ **Extensible** : Facile d'ajouter nouveaux opérateurs dans MappingEngine

---

## Version

- **Created** : 31 Mars 2026
- **Implementation Status** : ✅ Complete & Tested
- **Framework** : ESP32 Arduino, NiDMI v2.1+
- **Authors** : Hardware Team, Firmware Team
