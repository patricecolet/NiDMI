# Analyse détaillée du système de sauvegarde des paramètres

## Vue d'ensemble

Le système de sauvegarde des paramètres de composants fonctionne en plusieurs étapes :

1. **Frontend** : Lecture depuis le formulaire HTML → Stockage dans `pcfg` → Envoi au backend
2. **Backend** : Réception des paramètres → Construction JSON → Sauvegarde NVS → Mise à jour managers

---

## 1. Frontend : Lecture des paramètres

### 1.1 Types de paramètres

Le système gère plusieurs types de paramètres :

#### A. **formFields** (champs de formulaire spécifiques au composant)
- Définis dans `ComponentDefinition.formFields[]`
- Types : TEXT, NUMBER, SELECT, CHECKBOX, RANGE, INFO
- Exemples : `ledMode`, `btnMode`, `filterIntensity`, `min`, `max`
- **Lecture** : `component-config.js::readCfg()` lignes 113-131
- **Application** : `component-config.js::applyCfg()` lignes 201-215

#### B. **Paramètres MIDI** (dynamiques depuis `def.midiMessages[].params[]`)
- Définis dans `ComponentDefinition.midiMessages[].params[]`
- Exemples : `rtpCc`, `rtpNote`, `rtpChan`, `rtpVel`, `rtpCcMin`, `rtpCcMax`
- **Lecture** : `midi-config.js::readConfig()` (appelé depuis `readCfg()`)
- **Application** : `midi-config.js::applyConfig()` (appelé depuis `applyCfg()`)

#### C. **additionalPins** (pins additionnelles pour composants complexes)
- Définis dans `ComponentDefinition.additionalPins[]`
- Exemples : `sig`, `s0`, `s1`, `s2`, `s3`, `en` (pour MUX)
- **Lecture** : `component-config.js::readAdditionalPins()` lignes 11-58
- **Application** : `component-config.js::applyAdditionalPins()` lignes 66-89

#### D. **Paramètres communs** (OSC, Debug)
- Hardcodés : `oscEnabled`, `oscAddress`, `oscFormat`, `dbgEnabled`, `dbgHeader`
- **Lecture** : `component-config.js::readCfg()` lignes 140-145
- **Application** : `component-config.js::applyCfg()` lignes 222-227

### 1.2 Flux de lecture (`readCfg()`)

```javascript
// 1. Rôle du composant
c.role = roleOverride || $('#funcSelect')?.value || '';

// 2. formFields (dynamique depuis def.formFields)
def.formFields.forEach(field => {
  if (field.type === 3) { /* CHECKBOX */
    c[field.id] = el.checked;
  } else if (field.type === 4) { /* RANGE */
    c[field.id + 'Min'] = elMin.value;
    c[field.id + 'Max'] = elMax.value;
  } else {
    c[field.id] = el.value;
  }
});

// 3. Paramètres MIDI (dynamique depuis def.midiMessages[].params[])
MidiConfig.readConfig(def);

// 4. Paramètres communs (hardcodés)
c.oscEnabled = !!$('#oscEnabled2')?.checked;
c.oscAddress = $('#oscAddress')?.value || '';
// ...

// 5. additionalPins (dynamique depuis def.additionalPins)
readAdditionalPins(def, c);
```

### 1.3 Stockage dans `pcfg`

Tous les paramètres sont stockés dans l'objet global `pcfg` :
```javascript
pcfg[pinLabel] = {
  role: "potentiometer",
  ledMode: "pwm",
  rtpCc: 7,
  rtpChan: 1,
  oscEnabled: true,
  additionalPins: { sig: 4, s0: 5, s1: 6, ... },
  complexId: 0,
  // ...
}
```

---

## 2. Frontend : Envoi au backend (`saveAll()`)

### 2.1 Construction des paramètres URL

Le frontend construit un `URLSearchParams` avec tous les paramètres :

```javascript
// 1. Paramètres de base
p.set('pinLabel', lbl);
p.set('role', c.role);

// 2. Paramètres MIDI (dynamique depuis c)
Object.keys(c).forEach(key => {
  if(key.startsWith('rtp') && key !== 'rtpEnabled' && key !== 'rtpType') {
    p.set(key, c[key]);
  }
});

// 3. formFields (dynamique depuis def.formFields)
def.formFields.forEach(field => {
  if (field.type === 3) { /* CHECKBOX */
    if(value === true || value === 'true') {
      p.set(field.id, 'true');
    }
  } else if (field.type === 4) { /* RANGE */
    p.set(field.id + 'Min', c[field.id + 'Min']);
    p.set(field.id + 'Max', c[field.id + 'Max']);
  } else {
    p.set(field.id, value);
  }
});

// 4. additionalPins (dynamique depuis def.additionalPins)
def.additionalPins.forEach(pinDef => {
  p.set(pinDef.id, c.additionalPins[pinDef.id]);
});

// 5. Paramètres communs (hardcodés)
p.set('oscEnabled', 'true');
p.set('oscAddress', c.oscAddress);
// ...
```

### 2.2 Envoi HTTP POST

```javascript
fetch('/api/pins/set', {
  method: 'POST',
  headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
  body: p.toString()
});
```

---

## 3. Backend : Réception et sauvegarde (`/api/pins/set`)

### 3.1 Construction du JSON

Le backend construit un JSON à partir des paramètres reçus :

```cpp
// 1. Paramètres de base
json += "\"pinLabel\":\"" + pinLabel + "\"";
json += "\"role\":\"" + role + "\"";

// 2. Paramètres MIDI (HARDCODÉS - PROBLÈME)
addParam("rtpEnabled");
addParam("rtpType");
addParam("rtpNote");
addParam("rtpCc");
// ... (liste complète hardcodée lignes 254-277)

// 3. formFields (HARDCODÉS - PROBLÈME)
addParam("ledMode");
addParam("btnMode");
addParam("btnPulseTiming");
addParam("filterIntensity");
// ... (liste partielle hardcodée)

// 4. additionalPins (DYNAMIQUE - OK)
if(def && def->additionalPinCount > 0) {
  for(uint8_t i = 0; i < def->additionalPinCount; i++) {
    const AdditionalPinDef& pin = def->additionalPins[i];
    if(request->hasParam(pin.id, true)) {
      json += "\"" + String(pin.id) + "\":" + request->getParam(pin.id, true)->value();
    }
  }
}

// 5. Paramètres communs (HARDCODÉS - OK car communs)
addParam("oscEnabled");
addParam("oscAddress");
addParam("oscFormat");
addParam("dbgEnabled");
addParam("dbgHeader");
```

### 3.2 Sauvegarde NVS

```cpp
Preferences preferences;
preferences.begin("nidmi", false);
String key = "pin_" + pinLabel;
preferences.putString(key.c_str(), json);
preferences.end();
```

### 3.3 Sauvegarde dans managers (pour composants complexes)

Pour les MUX, le backend sauvegarde aussi dans `MuxManager` :
- Lire dynamiquement les `additionalPins` depuis la requête
- Mapper vers les paramètres de `addMux()`
- Sauvegarder dans `MuxManager` et NVS (clés `mux_X`, `mux_thresh_X`)

---

## 4. Problèmes identifiés

### 4.1 ❌ Backend : Paramètres MIDI hardcodés

**Fichier** : `src/api/PinAPI.cpp` lignes 254-268

**Problème** : Le backend a une liste hardcodée de paramètres MIDI au lieu de lire dynamiquement depuis `def->midiMessages[].params[]`.

**Impact** : Si un nouveau composant ajoute un nouveau paramètre MIDI, il ne sera pas sauvegardé par le backend.

**Solution** : Généraliser pour lire dynamiquement depuis `def->midiMessages[].params[]`.

### 4.2 ❌ Backend : formFields partiellement hardcodés

**Fichier** : `src/api/PinAPI.cpp` lignes 269-272

**Problème** : Seulement quelques `formFields` sont hardcodés (`ledMode`, `btnMode`, `btnPulseTiming`, `filterIntensity`). Les autres ne sont pas sauvegardés.

**Impact** : Si un composant ajoute un nouveau `formField`, il ne sera pas sauvegardé par le backend.

**Solution** : Généraliser pour lire dynamiquement depuis `def->formFields[]`.

### 4.3 ✅ Frontend : Lecture dynamique complète

**Statut** : Le frontend lit déjà dynamiquement tous les types de paramètres depuis les définitions.

### 4.4 ✅ Frontend : Envoi dynamique complet

**Statut** : Le frontend envoie déjà dynamiquement tous les types de paramètres au backend.

### 4.5 ✅ Backend : additionalPins dynamiques

**Statut** : Le backend lit déjà dynamiquement les `additionalPins` depuis `def->additionalPins[]`.

---

## 5. Solutions proposées

### 5.1 Généraliser la lecture des paramètres MIDI dans le backend

**Fichier** : `src/api/PinAPI.cpp`

**Avant** :
```cpp
addParam("rtpEnabled");
addParam("rtpType");
addParam("rtpNote");
// ... (liste hardcodée)
```

**Après** :
```cpp
// Lire dynamiquement depuis def->midiMessages[].params[]
if(def && def->midiMessageCount > 0 && def->midiMessages) {
  for(uint8_t i = 0; i < def->midiMessageCount; i++) {
    const MidiMessageDef& msg = def->midiMessages[i];
    if(msg.params && msg.paramCount > 0) {
      for(uint8_t j = 0; j < msg.paramCount; j++) {
        const MidiParamDef& param = msg.params[j];
        if(param.id) {
          addParam(param.id);
        }
      }
    }
  }
}
// Paramètres MIDI communs (hardcodés car toujours présents)
addParam("rtpEnabled");
addParam("rtpType");
```

### 5.2 Généraliser la lecture des formFields dans le backend

**Fichier** : `src/api/PinAPI.cpp`

**Avant** :
```cpp
addParam("ledMode");
addParam("btnMode");
addParam("btnPulseTiming");
addParam("filterIntensity");
```

**Après** :
```cpp
// Lire dynamiquement depuis def->formFields[]
if(def && def->formFieldCount > 0 && def->formFields) {
  for(uint8_t i = 0; i < def->formFieldCount; i++) {
    const FormFieldDef& field = def->formFields[i];
    if(field.id && !field.id.startsWith('_')) {
      if(field.type == FieldType::CHECKBOX) {
        // Gérer checkbox
        if(request->hasParam(field.id, true)) {
          String val = request->getParam(field.id, true)->value();
          if(val == "true") {
            addParam(field.id);
          }
        }
      } else if(field.type == FieldType::RANGE) {
        // Gérer range (Min/Max)
        addParam(String(field.id) + "Min");
        addParam(String(field.id) + "Max");
      } else {
        // Gérer autres types
        addParam(field.id);
      }
    }
  }
}
```

---

## 6. Résumé

### ✅ Ce qui fonctionne bien

1. **Frontend** : Lecture et envoi dynamiques complets
2. **Backend** : `additionalPins` dynamiques
3. **Architecture** : Séparation claire entre types de paramètres

### ❌ Ce qui doit être corrigé

1. **Backend** : Paramètres MIDI hardcodés → Généraliser
2. **Backend** : `formFields` partiellement hardcodés → Généraliser

### 📊 Impact

- **Actuel** : Les nouveaux paramètres MIDI et `formFields` ne sont pas sauvegardés par le backend
- **Après correction** : Tous les paramètres seront sauvegardés dynamiquement, permettant l'ajout de nouveaux composants sans modification du code de sauvegarde
