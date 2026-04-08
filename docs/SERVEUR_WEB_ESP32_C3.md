# Serveur web NiDMI sur ESP32‑C3 : mémoire, API définitions et JSON compact

Ce document décrit les contraintes mémoire du **ESP32‑C3**, les symptômes observés sur l’interface web, les causes techniques et les choix retenus (pagination, buffer, format JSON, frontend). Il sert de référence pour maintenir le firmware et l’UI sans réintroduire les régressions.

## 1. Contexte matériel et logiciel

### Mémoire sur C3

- La RAM « dynamique » (heap + données globales) est **limitée** (ordre de grandeur : ~400 Ko utiles selon partition et core).
- **Le BSS (variables globales statiques) et le heap partagent la même DRAM.** Chaque kilo-octet ajouté en `static char buffer[N]` **réduit d’autant** le heap disponible pour WiFi, TCP, TLS éventuel, WebSocket et allocations ponctuelles.
- Après démarrage WiFi + `ESPAsyncWebServer`, il reste typiquement **~25–37 Ko** de heap libre selon la configuration (taille des buffers statiques, RTP‑MIDI, tâches FreeRTOS, etc.).

### Chaîne HTTP côté firmware

- Les définitions de composants sont servies par **`GET /api/components/definitions`** avec pagination (`NIDMI_COMPONENT_DEFS_PAGINATION`).
- La réponse est construite dans un buffer puis envoyée via **`beginResponse_P`** (lecture depuis la mémoire sans copie du corps dans un `String` Arduino).

## 2. Symptômes observés (avant les correctifs)

- **Pagination incomplète** : le navigateur affichait « 32/41 » ou « 39/41 » composants chargés, avec des pages « irrécupérables ».
- **Erreurs JSON côté navigateur** : `Unexpected end of JSON input` sur d’autres routes si la réponse était vide ou tronquée.
- **Page web qui ne se charge pas** ou **serveur instable** : en réalité souvent une combinaison de **heap trop bas** + réponses invalides, pas un « crash WiFi » isolé.

## 3. Causes principales

### 3.1 Copie du corps de réponse en heap (`beginResponse` + `String`)

`request->beginResponse(200, "application/json", buffer)` copie le JSON dans un **`String`** interne à la réponse. Pour un corps de **~8 Ko**, il faut **~8 Ko contigus** supplémentaires au moment de la copie.

- Avec **~11–20 Ko** de heap libre pendant les requêtes, la copie peut **échouer silencieusement** (corps vide ou invalide) → le frontend retente indéfiniment certaines pages.
- **Leçon :** sur C3, éviter les grosses copies heap pour les réponses API ; préférer **`beginResponse_P`** (ou équivalent qui ne duplique pas tout le corps en RAM dynamique).

### 3.2 Plusieurs composants par page et « pics » de taille JSON

Avec **`limit = 3`** (trois définitions par page), une page peut regrouper trois composants **très verbeux** en JSON (ex. capteurs avec beaucoup de champs MIDI et de listes d’options). Une telle page dépassait **8 Ko** de JSON utile.

- Même avec un buffer de sérialisation assez grand, la **copie String** restait le point faible (voir §3.1).
- **Leçon :** borner la taille **maximale** d’une page en ne mettant **qu’un composant par requête** côté serveur (`limit = 1` imposé), quitte à multiplier le nombre de requêtes (~41 pages pour 41 composants).

### 3.3 Buffer statique trop gros vs buffer trop petit

- Un **gros** `static char jsonBuffer[12288]` **améliore** la sérialisation (pas de malloc, pas de copie String si `beginResponse_P`) mais **mange** le heap sous forme de BSS.
- Un **petit** buffer (ex. 8 Ko) sans réduction du JSON : le composant le plus lourd (**LIS3DH**) ne tenait pas → page « vide » `[]` ou erreur 500.

**Équilibre retenu :** JSON **compact** (voir §4) pour que le plus gros composant tienne dans **10 Ko** de buffer statique, avec **`beginResponse_P`**.

### 3.4 Fuite mémoire (correctif annexe)

Une fuite dans l’enregistrement des définitions (`ComponentRegistry`) pouvait **aggraver** la situation. Elle a été corrigée (`cleanup()` sur les définitions dupliquées). Sans ça, le heap se dégrade au fil du temps.

## 4. Optimisation du JSON des définitions

### 4.1 Objectifs

- Réduire la **taille sur le fil** et dans le **buffer de sérialisation**.
- Garder le **reste du frontend** inchangé en termes de propriétés attendues (`displayName`, `formFields`, etc.).

### 4.2 Clés courtes (côté firmware)

La méthode `ComponentDefinition::toJson()` émet un JSON avec des **clés abrégées**, par exemple :

| Compact (API) | Propriété logique (UI) |
|---------------|-------------------------|
| `dn` | `displayName` |
| `ff` | `formFields` |
| `mm` | `midiMessages` |
| `cid` | `cardId` |
| `fn` | `familyName` |
| `pt` / `apt` | `pinType` / `altPinType` |
| `impl`, `midi`, `osc` | `implemented`, `supportsMidi`, `supportsOsc` |
| `ap` / `apc` | `additionalPins` / `additionalPinCount` |
| `st` / `stt` | `statusTemplate` (message) / `statusTextTemplate` (composant) |
| `p` | `params` (dans un message MIDI) |
| `ph`, `dv` | `placeholder`, `defaultValue` |
| `o` | `options` (voir §4.3) |
| … | (autres : `dep`, `hp`, `hc`, `sep`, `don`, `sw`, `w`, etc.) |

La table complète est commentée dans **`src/components/ComponentDefinition.h`** au-dessus de `toJson()`.

### 4.3 Options des champs SELECT : tableau inline

Auparavant, les options étaient une **chaîne** contenant du JSON déjà stringifié, ce qui imposait un **double échappement** des guillemets (`\"` partout) et gonflait fortement la taille.

Désormais, le firmware émet **`"o":[...]`** : un **vrai tableau JSON** dans l’objet champ, sans couche string. Le parseur `JSON.parse` du navigateur produit directement un tableau d’objets `{ value, label }`.

### 4.4 Normalisation côté navigateur

Le fichier **`web/js/definitions.js`** définit **`normalizeDef()`**, appelée **une fois par composant** lors du chargement paginé (et en mode non paginé). Elle reconstruit un objet avec les **noms longs** attendus par `form-generator.js`, `component-menus.js`, etc.

Ainsi, seul le chemin « chargement des définitions » connaît le format compact ; le reste du code continue d’utiliser `displayName`, `formFields`, `options` comme tableau, etc.

## 5. Limites et comportement de l’API `/api/components/definitions`

### 5.1 `limit` imposé à 1

- Le paramètre **`limit`** de la requête est **ignoré** pour la taille de page : le serveur force **`limit = 1`** (un composant par page).
- **Pourquoi :** garantir une borne supérieure sur la taille du JSON par réponse et rester compatible avec un **buffer statique de 10 Ko** après compactage.
- **Inconvénient accepté :** ~41 requêtes HTTP pour 41 composants ; le frontend introduit un **délai court** entre requêtes pour ne pas saturer la pile TCP / le heap.

### 5.2 En-têtes de pagination

Le serveur renvoie notamment :

- `X-Total-Count` : nombre total de définitions  
- `X-Total-Pages` : égal au total (avec `limit = 1`)  
- `X-Current-Page` : index de page  
- `X-Per-Page` : toujours `1` dans cette configuration  

Le frontend lit **`X-Per-Page`** pour calculer le nombre de pages et enchaîne les `fetch`.

### 5.3 Buffer et envoi

- **Taille du buffer :** `10240` octets dans **`src/api/ComponentsAPI.cpp`** (zone paginée).
- **Condition de succès :** la sérialisation doit produire un JSON **non vide** et **strictement inférieur** à la capacité du buffer (marge pour `]` et `\0`).
- **Envoi :** `beginResponse_P(200, "application/json", ptr, len)` pour **ne pas** allouer une copie heap du corps entier.

## 6. Bluetooth et heap

Le **Bluetooth MIDI** n’est pas compilé par défaut (`NIDMI_ENABLE_BLE_MIDI` absent) : le message série indique que le BLE est désactivé. Ce n’est **pas** le principal consommateur de heap par rapport à **WiFi + TCP + serveur async** ; désactiver le BLE ne remplace pas les mesures ci‑dessus pour l’API définitions.

## 7. Fichiers concernés (référence rapide)

| Fichier | Rôle |
|---------|------|
| `src/components/ComponentDefinition.h` | `toJson()` compact + options inline |
| `src/api/ComponentsAPI.cpp` | Pagination, `limit = 1`, buffer 10 Ko, `beginResponse_P` |
| `web/js/definitions.js` | `normalizeDef()`, chargement paginé, délais / retries |
| `src/components/ComponentRegistry.cpp` | Enregistrement des définitions, `cleanup()` anti-fuite |

## 8. Vérification manuelle

1. Flasher un **ESP32‑C3** avec `./scripts/nidmi.sh upload --board c3` (adapter `--port`).
2. Se connecter au WiFi AP du device, ouvrir l’interface web.
3. Console navigateur : message du type **Total chargé: 41/41** et liste d’IDs cohérente.
4. Optionnel : moniteur série — lignes `[API] page=…` pour les pages 0 à 40 sans erreurs répétées sur une même page.

---

*Document aligné sur l’état du dépôt après stabilisation C3 (pagination, JSON compact, buffer 10 Ko, `beginResponse_P`).*
