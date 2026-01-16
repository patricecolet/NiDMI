# Refactorisation structure JavaScript - Plan d'action

## Objectifs

1. **Modulariser** le code JavaScript (actuellement 1760+ lignes dans `components.js`)
2. **Éliminer le hardcoding** (plus de "mux", "rtp", etc. codés en dur)
3. **Améliorer la lisibilité** et la maintenabilité
4. **Rendre le système générique** pour tous les composants complexes

## Analyse de la situation actuelle

### Problèmes identifiés

- **`components.js`** : 1760 lignes, mélange plusieurs responsabilités
- **`mux.js`** : 322 lignes, spécifique aux multiplexeurs (devrait être générique)
- **Hardcoding partout** : `rtpEnabled`, `muxSig`, `muxS0-S3`, `family === 1`, etc.
- **Variables incohérentes** : `rtpType` devrait être `midiMessageType`, `rtpCc` → `midiCc`, etc.
- **Pas de séparation claire** : configuration MIDI, génération de formulaires, gestion GPIO, etc.

### Avant vs Après

| Avant | Après |
|-------|-------|
| 1 fichier `components.js` (1760 lignes) | 7-8 modules (~300 lignes chacun) |
| Tout mélangé | Responsabilités séparées |
| Hardcoding partout | Système générique basé sur définitions |
| Variables incohérentes | Variables cohérentes et génériques |

## Structure proposée

```
web/js/
├── core.js                    (~100 lignes - utilitaires de base : $, helpers)
│   └── Fonction $() et helpers basiques
│
├── api.js                     (~400 lignes - appels API HTTP uniquement)
│   └── loadComponentDefinitions()
│   └── loadCaps(), loadConfiguredPins()
│   └── savePinConfig(), saveAll()
│   └── Tous les appels fetch()
│
├── definitions.js             (~200 lignes - gestion des définitions backend)
│   └── ComponentDefinitions.cache
│   └── ComponentDefinitions.getById(id)
│   └── ComponentDefinitions.getByFamily(familyId)
│   └── ComponentDefinitions.getForPinType(pinType)
│   └── ComponentDefinitions.load()
│
├── form-generator.js          (~400 lignes - génération HTML formulaires)
│   └── FormGenerator.getFieldId(def, fieldId)  // IDs dynamiques
│   └── FormGenerator.generateFormFields(def, containerId, cfg)
│   └── FormGenerator.generateAdditionalPins(def, containerId, cfg)
│   └── Logique d'affichage conditionnel (dependsOn, showWhen)
│
├── midi-config.js             (~300 lignes - configuration messages MIDI générique)
│   └── MidiConfig.FIELDS { messageType, params, section }
│   └── MidiConfig.generateMessageSection(def, cfg, containerId)
│   └── MidiConfig.generateParams(def, container, cfg)
│   └── MidiConfig.updateVisibility()
│   └── MidiConfig.readConfig(def)  // Lit dynamiquement tous les paramètres
│   └── MidiConfig.applyConfig(cfg, def)  // Applique dynamiquement tous les paramètres
│   └── Variables: midiMessageType, midiCc, midiNote, midiChannel, etc.
│
├── complex-components.js      (~400 lignes - composants complexes génériques)
│   └── ComplexComponents.initForm(pinLabel)
│   └── ComplexComponents.loadConfig(component)
│   └── ComplexComponents.saveFromPin()
│   └── ComplexComponents.deleteComponent(id)
│   └── ComplexComponents.loadList()  // remplace loadMuxList()
│   └── Fusionne tout mux.js dedans
│
├── gpio-manager.js            (~300 lignes - gestion GPIOs, disponibilité)
│   └── GpioManager.getUsedGpios(additionalSelectIds)
│   └── GpioManager.getPinsByType(pinType, excludeGpios)
│   └── GpioManager.calculateAddressPins(sigGpio, usedGpios)
│   └── GpioManager.areAddressPinsAvailable(sigGpio)
│   └── GpioManager.getAvailableDigitalPins(usedGpios)
│   └── GpioManager.checkAutoAvailability(sigGpio, usedGpios)
│
├── components.js              (~300 lignes - orchestration UI uniquement)
│   └── showRoleCards(role, cfg)  // orchestre la génération
│   └── populateFamilySelect(pinType, pin)
│   └── populateComponentSelect(familyId, pinType, pin)
│   └── updFunc(lbl)  // mise à jour UI
│   └── readCfg()  // lit tous les champs (utilise les modules)
│   └── applyCfg(cfg)  // applique la config (utilise les modules)
│   └── getAllFieldIds()  // collecte tous les IDs (utilise les modules)
│
└── pins.js                    (~500 lignes - affichage board SVG, visuels)
    └── drawBoard()
    └── updatePinsList()
    └── stat(cfg, pinLabel), getComponentStatusText(def, cfg, pinLabel)
    └── updateBusVisuals()
    └── pType(lbl), getRoleDisplayLabel(role)
    └── setOptions(sel, options, pre)
```

## Principes de conception

### 1. Interfaces MIDI vs Configuration MIDI

**Important** : MIDI est indissociable du projet. Ce qu'on active/désactive, ce sont les **interfaces MIDI**.

- **Interfaces MIDI** (checkboxes simples dans HTML, codées en dur OK) :
  - `rtpMidiEnabled` - RTP-MIDI
  - `usbMidiEnabled` - USB-MIDI (futur)
  - `debugMidiEnabled` - Debug console MIDI
  
- **Configuration MIDI** (généré dynamiquement via `midi-config.js`) :
  - `midiMessageType` - Type de message (Control Change, Note On, etc.)
  - Paramètres dynamiques basés sur `def.midiMessages[].params[]` :
    - Exemples : `midiCc`, `midiNote`, `midiPc`, `midiChannel`, `midiVelocity`, `midiRange`, etc.
    - **Tous les paramètres sont lus dynamiquement depuis les définitions backend**

### 2. Système générique pour composants complexes

- **Plus de hardcoding** : tout basé sur `ComponentDefinitions`
- **IDs dynamiques** : `FormGenerator.getFieldId(def, fieldId)` au lieu de "muxS0" hardcodé
- **Filtrage générique** : `isComplex` au lieu de `family === 1`
- **Une seule fonction** pour tous les composants complexes, pas une par type

### 3. Séparation claire des responsabilités

| Module | Responsabilité |
|--------|---------------|
| `definitions.js` | Cache et accès aux définitions backend |
| `form-generator.js` | Génération HTML (formFields, additionalPins) |
| `midi-config.js` | Configuration messages MIDI uniquement |
| `complex-components.js` | Logique métier composants complexes |
| `gpio-manager.js` | Gestion disponibilité et calculs GPIOs |
| `components.js` | Orchestration UI (coordonne les modules) |
| `pins.js` | Affichage visuel (SVG board, listes) |

## Renommage des variables (RTP → MIDI)

### Variables MIDI génériques

| Ancien (RTP) | Nouveau (MIDI générique) |
|-------------|-------------------------|
| `rtpType` / `rtpMsgType` | `midiMessageType` |
| `rtpCc` | `midiCc` |
| `rtpNote` | `midiNote` |
| `rtpPc` | `midiPc` |
| `rtpChan` | `midiChannel` |
| `rtpVelocity` | `midiVelocity` |
| `rtpParams` | `midiParams` |
| `rtpMidiSection` | `midiMessageSection` |

### Interfaces MIDI (codées en dur dans HTML)

```html
<input type="checkbox" id="rtpMidiEnabled">  <!-- RTP-MIDI -->
<input type="checkbox" id="usbMidiEnabled">  <!-- USB-MIDI -->
<input type="checkbox" id="debugMidiEnabled"> <!-- Debug MIDI -->
```

### Fonctions renommées

| Ancien | Nouveau |
|--------|---------|
| `generateRtpMidiSection()` | `MidiConfig.generateMessageSection()` |
| `generateRtpParams()` | `MidiConfig.generateParams()` |
| `updateRtpParamsVisibility()` | `MidiConfig.updateVisibility()` |
| `updateRtpForRole()` | `MidiConfig.updateForRole()` (ou supprimé si redondant) |

## Structure HTML (après refactorisation)

```html
<div id="componentFormCard">
  <!-- Configuration messages MIDI (généré dynamiquement) -->
  <h4>MIDI</h4>
  <div id="midiMessageSection"></div>
  
  <!-- Interfaces MIDI (checkboxes simples, codées en dur) -->
  <h4>Interfaces MIDI</h4>
  <div class="r switch">
    <input type="checkbox" id="rtpMidiEnabled">
    <label for="rtpMidiEnabled">RTP-MIDI</label>
  </div>
  <div class="r switch">
    <input type="checkbox" id="usbMidiEnabled">
    <label for="usbMidiEnabled">USB-MIDI</label>
  </div>
  <div class="r switch">
    <input type="checkbox" id="debugMidiEnabled">
    <label for="debugMidiEnabled">Debug MIDI</label>
  </div>
  
  <!-- OSC (codé en dur, OK) -->
  <h4>OSC</h4>
  <div class="r switch">
    <input type="checkbox" id="oscEnabled2">
    <label for="oscEnabled2">Activer</label>
    <label>Adresse:</label>
    <input type="text" id="oscAddress" placeholder="/ctl">
  </div>
  
  <!-- Debug (codé en dur, OK) -->
  <h4>Debug</h4>
  <div class="r switch">
    <input type="checkbox" id="dbgEnabled">
    <label for="dbgEnabled">Activer</label>
    <label>Header:</label>
    <input type="text" id="dbgHeader" placeholder="[DBG]">
  </div>
</div>
```

## Plan de migration progressif

### Phase 1 : Créer les nouveaux modules (sans modifier l'existant)

**1.1 Créer `definitions.js`**
- Extraire les fonctions d'accès aux définitions depuis `components.js`
- Créer `ComponentDefinitions` comme objet/namespace
- Tester que ça fonctionne avec l'existant

**1.2 Créer `form-generator.js`**
- Extraire `generateFormFields()`, `generateAdditionalPins()`, `getFieldId()`
- Créer `FormGenerator` comme namespace
- Tester que ça fonctionne avec l'existant

**1.3 Créer `midi-config.js`**
- Créer `MidiConfig` avec `generateMessageSection()`, `generateParams()`
- **Renommer** : `rtpType` → `midiMessageType`, `rtpCc` → `midiCc`, etc.
- Garder compatibilité temporaire (wrapper qui appelle les anciennes fonctions)
- Tester que ça fonctionne

**1.4 Créer `gpio-manager.js`**
- Extraire toutes les fonctions GPIO depuis `components.js`
- Créer `GpioManager` comme namespace
- Tester que ça fonctionne avec l'existant

### Phase 2 : Fusionner mux.js dans complex-components.js

**2.1 Créer `complex-components.js`**
- Déplacer les fonctions depuis `mux.js`
- Rendre générique (utiliser `definitions.js`, `form-generator.js`)
- Renommer : `loadMuxList()` → `ComplexComponents.loadList()`
- Renommer : `initMuxFormForPin()` → `ComplexComponents.initForm()`
- Tester que tout fonctionne

**2.2 Supprimer `mux.js`**
- Vérifier qu'il n'y a plus de références
- Supprimer le fichier

### Phase 3 : Refactoriser components.js

**3.1 Utiliser les nouveaux modules dans `components.js`**
- Remplacer les appels directs par les modules
- `showRoleCards()` utilise `FormGenerator`, `MidiConfig`
- `readCfg()` utilise `MidiConfig.readConfig(def)` (passe la définition pour lire dynamiquement)
- `applyCfg()` utilise `MidiConfig.applyConfig(cfg, def)` (passe la définition pour appliquer dynamiquement)
- Tester à chaque étape

**3.2 Simplifier `components.js`**
- Ne garder que l'orchestration
- Supprimer les fonctions déplacées
- Nettoyer le code

### Phase 4 : Nettoyage final

**4.1 Éliminer le hardcoding restant**
- Remplacer tous les IDs hardcodés par `FormGenerator.getFieldId()`
- Remplacer `family === 1` par `isComplex`
- Remplacer préfixes "mux" par IDs dynamiques

**4.2 Mettre à jour HTML**
- Ajouter les checkboxes interfaces MIDI (`rtpMidiEnabled`, etc.)
- Vérifier que tous les IDs correspondent

**4.3 Tests finaux**
- Tester tous les composants (pot, button, LED, MUX)
- Vérifier que la configuration se sauvegarde correctement
- Vérifier que l'UI s'affiche correctement

## Vérification de la complexité

### Avant refactorisation
- ❌ 1 fichier de 1760 lignes = très difficile à naviguer
- ❌ Responsabilités mélangées = difficile à modifier
- ❌ Hardcoding partout = difficile à étendre
- ❌ Pas de réutilisabilité

### Après refactorisation
- ✅ 7-8 modules de ~300 lignes = facile à naviguer
- ✅ Responsabilité unique par module = facile à modifier
- ✅ Système générique = facile à étendre
- ✅ Réutilisabilité = modules indépendants

### Risques et mitigations

| Risque | Mitigation |
|--------|-----------|
| Trop de petits fichiers | 300 lignes par module est un bon compromis (lisible mais pas trop fragmenté) |
| Dépendances entre modules | Interface claire (namespace objects), documentation dans chaque module |
| Migration difficile | Plan progressif, tester à chaque étape, garder compatibilité temporaire |
| Régression bugs | Tests à chaque phase, vérifier que tout fonctionne avant de continuer |

## Structure des modules (exemples)

### `midi-config.js` - Exemple

```javascript
/**
 * Configuration des messages MIDI (générique)
 * RTP-MIDI, USB-MIDI sont des interfaces activables via checkboxes HTML
 */
const MidiConfig = {
  FIELDS: {
    messageType: 'midiMessageType',
    params: 'midiParams',
    section: 'midiMessageSection'
  },
  
  /**
   * Génère la section de configuration des messages MIDI
   * Génère dynamiquement le select du type de message et tous les paramètres
   * depuis def.midiMessages[].params[] (pas limité à cc, note, channel - tous les paramètres)
   */
  generateMessageSection(def, currentCfg = {}, containerId = 'midiMessageSection') {
    // Select: midiMessageType (depuis def.midiMessages)
    // Container: midiParams (pour les paramètres générés dynamiquement)
    // Variables générées dynamiquement depuis def.midiMessages[].params[].id
    // Exemples possibles : midiCc, midiNote, midiChannel, midiVelocity, midiRange, etc.
  },
  
  /**
   * Lit la configuration MIDI depuis le formulaire
   * Lit dynamiquement tous les paramètres depuis les définitions du composant
   * (ne se limite pas à cc, note, channel - supporte tous les paramètres définis)
   */
  readConfig(def) {
    const config = {
      messageType: $('#' + this.FIELDS.messageType)?.value || '',
      // Interfaces MIDI (checkboxes HTML, codées en dur OK)
      rtpMidiEnabled: !!$('#rtpMidiEnabled')?.checked,
      usbMidiEnabled: !!$('#usbMidiEnabled')?.checked,
      debugMidiEnabled: !!$('#debugMidiEnabled')?.checked
    };
    
    // Lire dynamiquement tous les paramètres MIDI depuis les définitions
    // Parcourt tous les messages MIDI et leurs paramètres (cc, note, channel, velocity, range, etc.)
    if(def && def.midiMessages && Array.isArray(def.midiMessages)) {
      def.midiMessages.forEach(msg => {
        if(msg.params && Array.isArray(msg.params)) {
          msg.params.forEach(param => {
            if(param.id) {
              const el = $('#' + param.id);
              if(el) {
                if(param.type === 4) { // RANGE (velocity range, sweep range, etc.)
                  const elMin = $('#' + param.id + 'Min');
                  const elMax = $('#' + param.id + 'Max');
                  if(elMin) config[param.id + 'Min'] = elMin.value || '';
                  if(elMax) config[param.id + 'Max'] = elMax.value || '';
                } else {
                  config[param.id] = el.value || '';
                }
              }
            }
          });
        }
      });
    }
    
    return config;
  },
  
  /**
   * Applique la configuration MIDI au formulaire
   * Applique dynamiquement tous les paramètres depuis les définitions du composant
   * (ne se limite pas à cc, note, channel - supporte tous les paramètres définis)
   */
  applyConfig(cfg, def) {
    // Appliquer le type de message
    if(cfg.messageType || cfg.midiType) {
      setV(this.FIELDS.messageType, cfg.messageType || cfg.midiType);
    }
    
    // Appliquer les interfaces MIDI
    if(cfg.rtpMidiEnabled !== undefined) setC('rtpMidiEnabled', cfg.rtpMidiEnabled);
    if(cfg.usbMidiEnabled !== undefined) setC('usbMidiEnabled', cfg.usbMidiEnabled);
    if(cfg.debugMidiEnabled !== undefined) setC('debugMidiEnabled', cfg.debugMidiEnabled);
    
    // Appliquer dynamiquement tous les paramètres MIDI depuis les définitions
    // Parcourt tous les messages MIDI et leurs paramètres (cc, note, channel, velocity, range, etc.)
    if(def && def.midiMessages && Array.isArray(def.midiMessages)) {
      def.midiMessages.forEach(msg => {
        if(msg.params && Array.isArray(msg.params)) {
          msg.params.forEach(param => {
            if(param.id && cfg[param.id] !== undefined) {
              if(param.type === 4) { // RANGE
                if(cfg[param.id + 'Min'] !== undefined) setV(param.id + 'Min', cfg[param.id + 'Min']);
                if(cfg[param.id + 'Max'] !== undefined) setV(param.id + 'Max', cfg[param.id + 'Max']);
              } else {
                setV(param.id, cfg[param.id]);
              }
            }
          });
        }
      });
    }
    
    // Mettre à jour la visibilité des paramètres
    this.updateVisibility();
  },
  
  // ...
};
```

### `complex-components.js` - Exemple

```javascript
/**
 * Gestion des composants complexes (générique pour tous les types)
 * Remplace mux.js avec logique générique
 */
const ComplexComponents = {
  /**
   * Initialise le formulaire d'un composant complexe
   */
  initForm(pinLabel) {
    // Utilise ComponentDefinitions.getById()
    // Utilise FormGenerator.getFieldId()
    // Générique pour tous les composants complexes
  },
  
  /**
   * Charge la liste des composants complexes depuis le backend
   * (remplace loadMuxList)
   */
  async loadList() {
    // Appelle /api/mux/list (API backend encore spécifique, OK pour l'instant)
    // Utilise ComponentDefinitions pour déterminer le type
  },
  
  // ...
};
```

## Checklist de migration

- [ ] Phase 1.1 : Créer `definitions.js`
- [ ] Phase 1.2 : Créer `form-generator.js`
- [ ] Phase 1.3 : Créer `midi-config.js` (avec renommage variables)
- [ ] Phase 1.4 : Créer `gpio-manager.js`
- [ ] Phase 2.1 : Créer `complex-components.js` (fusionne mux.js)
- [ ] Phase 2.2 : Supprimer `mux.js`
- [ ] Phase 3.1 : Refactoriser `components.js` (utilise les modules)
- [ ] Phase 3.2 : Simplifier `components.js`
- [ ] Phase 4.1 : Éliminer hardcoding restant
- [ ] Phase 4.2 : Mettre à jour HTML (interfaces MIDI)
- [ ] Phase 4.3 : Tests finaux

## Estimation

- **Phase 1** : ~2-3h (création modules, tests)
- **Phase 2** : ~1h (fusion mux.js)
- **Phase 3** : ~2h (refactorisation components.js)
- **Phase 4** : ~1h (nettoyage, tests)

**Total estimé** : ~6-7h de travail

## Notes importantes

1. **Migration progressive** : Ne pas tout refactoriser d'un coup, étape par étape
2. **Tests à chaque phase** : Vérifier que tout fonctionne avant de continuer
3. **Compatibilité temporaire** : Garder les anciennes fonctions qui appellent les nouvelles si nécessaire
4. **Documentation** : Commenter chaque module avec sa responsabilité
5. **Pas de rush** : Mieux vaut prendre le temps et faire ça bien

## Conclusion

Cette structure est **plus claire** et **plus maintenable** que le monolithe actuel. 
Elle permet d'**éliminer le hardcoding** progressivement tout en gardant le code **lisible** et **extensible**.
