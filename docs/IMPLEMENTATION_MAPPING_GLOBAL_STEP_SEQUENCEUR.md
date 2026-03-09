# Implementation: Mapping Global + Step Sequenceur dans l'onglet Pins

## 1) Objectif produit

Ajouter une vue "globale" dans l'onglet `Pins`:

- clic sur un bouton central au milieu du MCU -> affiche la page "Mapping global + Step sequenceur"
- clic sur une pin (board SVG, liste des pins configurees, selecteurs de fonction) -> revient au formulaire pin classique

Le comportement attendu est un switch de panneau droit:

- mode `pin` (actuel): formulaire d'une pin (`family`, `composant`, MIDI/OSC/Debug)
- mode `global` (nouveau): configuration globale (mapping et sequenceur)

## 2) Etat actuel (base existante)

### Frontend

- `web/index.html`: panel `Pins` avec:
  - board SVG (`pinsLeft`, `pinsRight`)
  - formulaire pin unique dans la colonne droite (`.rp .cp`)
- `web/js/pin-visual.js`:
  - dessine les rectangles SVG de pins
  - sur clic pin: met `cur`, appelle `handlePinClick(label)`, `updFunc(label)`, puis `applyCfg(...)`
- `web/js/pin-list.js`:
  - clic sur item pin: sélection visuelle + `updFunc(lbl)` + `applyCfg(...)`
- `web/js/component-handlers.js`:
  - `updFunc(lbl)` initialise/restore les menus et le formulaire de pin
- `web/js/component-config.js`:
  - `readCfg()` / `applyCfg()` pour la config pin
- `web/js/api.js`:
  - save/load des pins via `/api/pins/*`

### Backend

- `src/api/PinAPI.cpp`:
  - `GET /api/pins/list`
  - `POST /api/pins/set`
  - `POST /api/pins/delete`
  - `GET /api/pins/caps`
- stockage NVS existant pour chaque pin: cle `pin_<label>`

## 3) Architecture cible

### 3.1 Etat UI unique pour le panneau droit

Ajouter un etat global UI:

- `pinsViewMode = 'pin' | 'global'`
- `selectedPinLabel = cur` (deja existant)

Rendre l'affichage du panneau droit pilotable uniquement par des fonctions centrales:

- `showPinEditor(pinLabel)`
- `showGlobalEditor()`
- `applyPinsViewMode()`

Regle:

- toute interaction pin appelle `showPinEditor(...)` avant/pendant `updFunc(...)`
- bouton central appelle `showGlobalEditor()`

### 3.2 Separation des donnees

Conserver 2 espaces de configuration:

- `pcfg` (existant): config par pin
- `gcfg` (nouveau): config globale mapping + sequenceur

`gcfg` est charge/sauve via API dediee, pas via `/api/pins/set`.

## 4) Modifications Frontend detaillees

## 4.1 `web/index.html`

### A. Bouton central sur le MCU

Dans le SVG du board, remplacer le rectangle central decoratif par un groupe cliquable:

- id recommande: `globalModeBtn`
- label recommande: `GLOBAL`
- style visuel actif/inactif

Exemple de structure:

- `<g id="globalModeBtn">`
  - `<rect id="globalModeBtnRect" ... />`
  - `<text ...>GLOBAL</text>`
- `</g>`

### B. Panneau droit en 2 vues

Dans la colonne droite (`.rp`), envelopper l'existant:

- conteneur pin existant -> `id="pinEditorCard"`
- nouveau conteneur global -> `id="globalEditorCard"` (cache par defaut)

Structure recommandee:

- `#pinEditorCard` -> formulaire actuel inchangé
- `#globalEditorCard` -> sections:
  - `Mapping global`
  - `Step sequenceur`
  - `Boutons Sauver/Recharger`
  - `Message statut`

### C. CSS

Ajouter classes:

- `.hidden { display:none !important; }`
- style bouton central:
  - normal
  - hover
  - actif (`.is-global-active`)

## 4.2 Nouveau module `web/js/pins-global.js`

Creer un module dedie pour isoler la logique globale:

- etat:
  - `let pinsViewMode = 'pin';`
  - `let gcfg = defaultGlobalConfig();`
- fonctions:
  - `defaultGlobalConfig()`
  - `applyPinsViewMode()`
  - `showPinEditor(pinLabel)`
  - `showGlobalEditor()`
  - `renderGlobalForm()`
  - `readGlobalForm()`
  - `loadGlobalConfig()`
  - `saveGlobalConfig()`
  - `initGlobalView()`

### Contrat de `applyPinsViewMode()`

- si `pinsViewMode === 'global'`:
  - afficher `#globalEditorCard`, cacher `#pinEditorCard`
  - ajouter classe active sur `#globalModeBtnRect`
- sinon:
  - inverse

### Contrat de `showPinEditor(pinLabel)`

- `pinsViewMode = 'pin'`
- `cur = pinLabel` (si fourni)
- `applyPinsViewMode()`

### Contrat de `showGlobalEditor()`

- `pinsViewMode = 'global'`
- `applyPinsViewMode()`
- precharger formulaire avec `gcfg`

## 4.3 `web/js/pin-visual.js`

### A. Hook bouton central

Dans `drawBoard()`, apres rendu SVG:

- recuperer `#globalModeBtn`
- ajouter `click` -> `showGlobalEditor()`

### B. Hook clic pin

Dans le handler de `mk(...).addEventListener('click')`:

- appeler `showPinEditor(label)` avant `updFunc(label)`
- conserver flux existant `handlePinClick`, `updFunc`, `applyCfg`

But: forcer le retour automatique au formulaire pin.

## 4.4 `web/js/pin-list.js`

Dans chaque `it.onclick` (simple + complex):

- appeler `showPinEditor(lbl)` avant `updFunc(lbl)`

## 4.5 `web/js/component-handlers.js`

Dans `updFunc(lbl)`, ajouter au debut une protection:

- si vue globale active, repasser en vue pin (`showPinEditor(lbl)`)

Cela evite les incoherences quand `updFunc` est appelee depuis d'autres endroits.

## 4.6 `web/app.js`

Au `DOMContentLoaded`:

- initialiser la vue globale (`initGlobalView()`)
- charger `gcfg` (`await loadGlobalConfig()`) apres chargement des caps/defs

Ordre recommande:

1. init tabs/forms existants
2. charger defs + caps + pins
3. `initGlobalView()`
4. `loadGlobalConfig()`
5. `applyPinsViewMode()` (mode initial `pin`)

## 4.7 Build system

Mettre le module `web/js/pins-global.js` dans l'ordre de concat:

- fichier `scripts/build_html_simple.sh`
- verifier que le nouveau JS est concatene avant `app.js`

Puis regen:

- `./scripts/build_html_simple.sh`
- verifier mise a jour `src/ui/ui_bundle.h` (attendue)

## 5) API Backend pour la config globale

## 5.1 Endpoints proposes

Dans `src/api/PinAPI.cpp` (ou nouveau `GlobalPinsAPI.cpp`):

- `GET /api/pins/global`
  - retourne `gcfg`
  - si absent en NVS -> retourner defaults

- `POST /api/pins/global`
  - accepte payload URL-encoded ou JSON
  - valide et sauvegarde en NVS
  - reponse `{ "status":"ok" }`

## 5.2 Cle NVS

Utiliser une cle dediee:

- namespace: `nidmi` (deja utilise)
- key: `pins_global_v1`

Avantage:

- versionnable
- ne perturbe pas les cles `pin_<label>`

## 5.3 Schema JSON recommande (v1)

```json
{
  "mapping": {
    "enabled": true,
    "source": "midi_cc",
    "target": "global_cc_bank_a",
    "mode": "scale",
    "inMin": 0,
    "inMax": 127,
    "outMin": 0,
    "outMax": 127
  },
  "sequencer": {
    "enabled": false,
    "clockSource": "internal",
    "tempo": 120,
    "steps": 16,
    "swing": 0,
    "direction": "forward",
    "gateMs": 80
  }
}
```

Notes:

- garder un schema simple au debut, extensible ensuite
- defaults robustes cote backend
- validation stricte des bornes numeriques

## 5.4 Validation backend minimale

- `tempo`: 20..300
- `steps`: 1..64
- `swing`: 0..75
- `gateMs`: 1..1000
- enums (`clockSource`, `direction`, etc.): whitelist

En cas d'erreur:

- HTTP 400 + message explicite

## 6) Flux UX cible (etat machine)

### 6.1 Entrer en mode global

1. utilisateur clique bouton central MCU
2. `showGlobalEditor()`
3. panneau droit -> global
4. formulaire global charge depuis `gcfg`

### 6.2 Retour au mode pin

Declencheurs:

- clic pin sur board
- clic pin dans liste configuree
- changement composant/famille (si besoin)

Actions:

1. `showPinEditor(label)`
2. `updFunc(label)`
3. `applyCfg(pcfg[label])` si existant

### 6.3 Persistance

- mode pin: persistance existante via `saveAll()`
- mode global: bouton dedie `Sauver global` -> `POST /api/pins/global`

Option future (non bloquante v1):

- integrer sauvegarde globale dans `saveAll()` en plus des pins

## 7) Plan d'implementation par etapes

## Etape 1 - UI structurelle

- ajouter bouton central cliquable
- ajouter `globalEditorCard` cache
- ajouter CSS d'etat

Critere OK:

- bouton visible/clicable
- switch visuel pin/global fonctionnel sans logique metier

## Etape 2 - Gestion d'etat frontend

- creer `pins-global.js`
- brancher `showGlobalEditor()` et `showPinEditor()`
- brancher clics pin/liste pour retour auto mode pin

Critere OK:

- toutes les interactions reviennent au bon panneau

## Etape 3 - API backend globale

- ajouter `GET/POST /api/pins/global`
- persister sous `pins_global_v1`
- valider schema + defaults

Critere OK:

- reboot conserve la config globale

## Etape 4 - Formulaire mapping/sequenceur

- generer champs
- binder `readGlobalForm()/renderGlobalForm()`
- save/reload

Critere OK:

- les valeurs sauvees reviennent a l'identique

## Etape 5 - Integration runtime (optionnelle selon sprint)

- connecter `gcfg` au moteur runtime (mapping + sequenceur reel)
- tracer logs debug

Critere OK:

- effets runtime observables sur MIDI/OSC selon specs

## 8) Checklist de tests

## 8.1 Tests UI

- clic bouton central -> mode global
- clic pin board -> mode pin + bonne pin selectionnee
- clic item liste pin -> mode pin + bonne config affichee
- toggle multiple global/pin sans erreur console

## 8.2 Tests API

- `GET /api/pins/global` retourne defaults si vide
- `POST /api/pins/global` avec payload valide -> 200
- payload invalide (tempo hors borne) -> 400
- reboot -> `GET` retourne config sauvegardee

## 8.3 Non-regression

- `saveAll()` des pins continue de fonctionner
- `loadConfiguredPins()` inchangé
- composants complexes (additionalPins) inchangés
- websocket `PIN_CLICKED` inchangé

## 9) Risques et mitigations

- conflit d'etat entre `cur` et vue globale
  - mitigation: fonctions uniques `showPinEditor/showGlobalEditor`
- regression sur rendu formulaire pin
  - mitigation: ne pas modifier `readCfg/applyCfg`, seulement le wrapper visuel
- oubli build bundle
  - mitigation: script unique `build_html_simple.sh` en fin de modif

## 10) Recommandation de livraison

- PR 1: infrastructure UI + etat + endpoint backend vide/default
- PR 2: schema complet + formulaire mapping/sequenceur + validation
- PR 3: integration runtime sequenceur (si hors scope UI)

Cette decomposition limite les regressions et permet de valider chaque couche separement.

