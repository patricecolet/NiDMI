# Plan d’implémentation : LittleFS pour le fichier séquenceur (`.nidmid`)

Ce document énumère les travaux à prévoir pour stocker le pseudo-MIDI du séquenceur dans une **partition flash dédiée**, montée en **LittleFS**, avec lecture/écriture côté firmware et échange via l’interface web NiDMI.

---

## 1) Objectif

- Persister sur la carte un fichier binaire (ex. `/seq/nidmid.bin` ou `/nidmid.bin`) produit par l’UI ou un outil externe.
- Le **moteur séquenceur** (firmware) lit ce fichier au démarrage ou à la demande.
- Les **mises à jour firmware (OTA)** ne doivent **pas** effacer la séquence si la partition data est séparée de la partition `factory`/`ota_*`.
- Rester cohérent avec l’existant NiDMI : **NVS** pour la configuration structurée ; **LittleFS** pour le **blob séquence** (fichier), pas pour remplacer toute la config.

---

## 2) Contexte actuel dans le dépôt

- Configuration pins, OSC, mux, etc. : **NVS** (`ConfigCache`, `ConfigLoader`, namespaces).
- Fichier `.nidmid` côté navigateur : parsing JS (`parseNidmid`), pas encore de persistance flash côté ESP pour ce contenu.
- Le projet documente des **tables de partitions** (ex. `tools/nidmi_c3_no_spiffs.csv` dans `GUIDE_SCRIPT_NIDMI.md`) : souvent **grande app**, **sans SPIFFS** historique — l’ajout LittleFS imposera de **réallouer l’espace flash** (réduire `app` ou utiliser une flash plus grande selon carte).

À vérifier sur la branche courante : présence réelle des CSV sous `tools/` et profils **C3** vs **S3**.

---

## 3) Architecture cible (aperçu)

| Élément | Rôle |
|--------|------|
| Partition `littlefs` (nom/subtype selon Arduino-ESP32) | Zone dédiée, taille fixe (ex. 256 Ko–1 Mo selon contraintes) |
| LittleFS monté au boot | `LittleFS.begin(true)` ou équivalent (voir doc Arduino pour formatage si partition vide) |
| Chemin logique unique | Ex. `/seq/nidmid.bin` — documenter la convention dans le code |
| API HTTP | Upload (POST), téléchargement (GET), métadonnées (taille, CRC optionnel), suppression optionnelle |
| NVS | Conserver `gcfg` (tempo, steps, etc.) ; **ne pas** dupliquer tout le binaire en NVS |

---

## 4) Liste de travaux (checklist)

### 4.1 Table des partitions et build

- [ ] **Définir la taille** cible du fichier séquenceur (ordre de grandeur max) + marge pour métadonnées futures.
- [ ] **Créer ou adapter un CSV** de partitions pour **chaque cible** (au minimum ESP32-C3 XIAO et ESP32-S3 XIAO si les deux sont supportées) :
  - [ ] Ajouter une sous-partition **data** de type LittleFS (subtype `spiffs`/`littlefs` selon convention **arduino-esp32** utilisée — à valider avec la version du core).
  - [ ] Vérifier la cohérence avec **OTA** (si deux slots `app`, la partition LittleFS reste en général **hors** des slots OTA).
  - [ ] Vérifier que la somme des tailles ne dépasse pas la **taille flash** de la carte.
- [ ] **Documenter** dans le README ou `GUIDE_SCRIPT_NIDMI.md` quel fichier CSV utiliser et comment le sélectionner dans Arduino IDE / `arduino-cli`.
- [ ] Mettre à jour **`scripts/nidmi.sh`** (ou équivalent) pour pointer vers le bon `--partition` si le script génère ou copie des binaires de partition.

### 4.2 Firmware — initialisation LittleFS

- [ ] Ajouter l’include et la dépendance au **core ESP32 Arduino** (`LittleFS.h` — API standard du package).
- [ ] Appeler le montage **tôt** dans `setup()` (avant ou avec le reste de l’init NiDMI), avec gestion d’erreur explicite (Serial + éventuellement statut web).
- [ ] Décider du comportement si **premier boot** / partition non formatée : format automatique (`begin(true)`) vs refus avec message — documenter le choix (impact sur données utilisateur).

### 4.3 Firmware — couche d’accès fichier

- [ ] Introduire un **module dédié** (ex. `SequencerFileStore` / `NidmidStorage`) pour :
  - [ ] Chemin canonique du fichier (constantes).
  - [ ] Écriture complète (upload) avec stratégie **fichier temporaire + rename** pour éviter un fichier tronqué en cas de coupure.
  - [ ] Lecture (flux ou chargement en RAM selon taille max prévue).
  - [ ] Suppression / réinitialisation optionnelle.
  - [ ] Optionnel : **CRC32** ou en-tête maison pour valider l’intégrité.
- [ ] Définir une **taille max** acceptée (refus au-delà) alignée sur la partition.

### 4.4 Firmware — API HTTP

- [ ] **`POST`** upload du binaire (body brut ou `multipart/form-data` — choisir une option et la documenter).
  - [ ] Authentification : aujourd’hui souvent réseau local ; noter si besoin d’un token plus tard.
- [ ] **`GET`** pour télécharger le fichier (ou une erreur 404 si absent).
- [ ] **`GET`** (JSON) métadonnées : taille, date d’écriture si disponible, version de format.
- [ ] **`DELETE`** optionnel pour effacer la séquence.
- [ ] Enregistrer les routes dans le module serveur existant (ex. à côté de `PinAPI.cpp` / `ServerCore` selon l’architecture actuelle).
- [ ] Gestion **mémoire** : upload par chunks ou buffer fixe pour ne pas saturer le heap.

### 4.5 Firmware — intégration moteur séquenceur

- [ ] Après chargement ou mise à jour du fichier, **informer** le composant séquenceur (callback, flag `reload`, ou lecture à la volée).
- [ ] Définir si le séquenceur lit **une fois** au boot ou **recharge** à chaud après POST réussi.
- [ ] Comportement si fichier **absent** ou **invalide** : défaut silencieux vs message utilisateur.

### 4.6 Frontend (web)

- [ ] Remplacer ou compléter le flux « fichier uniquement en mémoire navigateur » par un **`fetch` POST** vers la nouvelle API après sélection/validation du fichier.
- [ ] Afficher le **statut** : succès, erreur réseau, taille refusée.
- [ ] Optionnel : bouton **télécharger depuis la carte** (GET) pour sauvegarde locale.
- [ ] Rebuild du bundle : `scripts/build_html_simple.sh` après modification des JS.
- [ ] Mettre à jour `src/ui/ui_index.cpp` / `ui_bundle.h` générés si le flux UI change.

### 4.7 Documentation utilisateur

- [ ] Expliquer : **premier flash** avec nouvelle table de partitions peut **effacer** l’ancien contenu flash (comportement normal).
- [ ] Préciser que la **séquence** survit aux **OTA** si la partition data est inchangée (sous réserve de ne pas reflasher une image complète qui efface toute la flash).

### 4.8 Tests et validation

- [ ] Test sur **C3** et **S3** si les deux sont cibles.
- [ ] Upload d’un fichier de taille **0**, **maximale**, **au-delà de la limite** (rejet attendu).
- [ ] Coupure Wi-Fi pendant upload (fichier précédent intact si stratégie temporaire + rename).
- [ ] Vérifier **heap** / absence de fuite après plusieurs uploads.

### 4.9 Outils externes (optionnel)

- [ ] Vérifier avec **ESPConnect** (ou équivalent) si la partition LittleFS apparaît et si **upload/download** de fichiers est supporté pour cette configuration — non bloquant pour NiDMI, mais utile pour le support.

---

## 5) Ordre d’exécution recommandé

1. **Partitions** : CSV + build + flash de test (sketch minimal qui monte LittleFS et liste `/`).
2. **Module stockage** + **routes HTTP** minimales (POST/GET).
3. **Branchement UI** (upload + feedback).
4. **Moteur séquenceur** : lecture du fichier et comportement runtime.
5. **Durcissement** : limites, erreurs, doc, tests sur les deux MCU.

---

## 6) Points d’attention

- **arduino-esp32** : noms des types de partition et appels `LittleFS` peuvent varier légèrement selon la version du core — verrouiller la version dans la doc contributeur.
- **Espace** : sur XIAO, la flash est limitée ; chaque kilo-octet sur `app` ou la partition data compte.
- **Concurrence** : si le séquenceur lit pendant un upload, prévoir un verrou ou une reprise après écriture complète.

---

## 7) Références utiles (à consulter au moment de l’implémentation)

- Documentation **Arduino-ESP32** : *LittleFS*, *Partition Scheme*, *Flash Size*.
- Fichiers NiDMI existants : `src/api/PinAPI.cpp`, `src/server/`, `src/config/ConfigCache.*`.
- Document produit lié : `docs/IMPLEMENTATION_MAPPING_GLOBAL_STEP_SEQUENCEUR.md` (config globale `gcfg` vs fichier binaire).

---

*Document de planification — à mettre à jour au fur et à mesure des merges (cases cochées, noms de fichiers réels, contraintes de taille mesurées).*
