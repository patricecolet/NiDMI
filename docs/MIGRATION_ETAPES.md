## Vue d’ensemble

- **Branche source** : `feature/c3-partition-no-spiffs`  
- **Cible** : `main`  
- **Volume** : 90+ fichiers modifiés (C++, scripts, drivers I2C, processeurs, UI web, docs, CSV de partitions…)

- **Stratégie globale** :
  - **Créer une sous-branche par “step”** depuis `main`.
  - **Chaque step** cherry-picke ou réapplique seulement un **sous-ensemble cohérent** des changements.
  - **1 PR par step**, revue + tests, puis merge dans `main`.
  - Nous sommes **3 personnes** : chacun·e peut prendre une step (ou plusieurs), en parallèle quand c’est raisonnable.

---

## Étape 0 – Préparation commune

- **Objectifs**
  - **S’assurer que tout le monde part du même état.**

- **Actions**
  - `git checkout main && git pull`
  - `git checkout feature/c3-partition-no-spiffs && git pull` (si remote à jour)
  - Décider si l’on fait :
    - **plutôt des cherry-pick** depuis `feature/c3-partition-no-spiffs`, ou
    - une **ré-implémentation guidée par diff** (utile si l’historique de la feature est un peu “brouillon”).

---

## Étape 1 – Docs & scripts (Personne 1)

- **Branche** : `feature/c3-step1-docs-tools` (depuis `main`)

- **Fichiers concernés (principaux)**
  - `docs/GUIDE_SCRIPT_NIDMI.md`
  - `docs/OPTIMISATION_TOUCH_S3.md`
  - `docs/PROPOSITIONS_ARCHITECTURE.md`
  - `scripts/nidmi.sh`
  - `tools/README.md`
  - `pd/basic-exemple.pd`

- **Tâches**
  - **Reprendre/corriger le contenu des docs** (vérifier qu’elles sont alignées avec l’état actuel de `main`).
  - **Intégrer les améliorations de** `scripts/nidmi.sh` (build/monitor/upload, options C3, etc.) en veillant à ne pas casser les usages actuels.
  - **Ajouter/mettre à jour les fichiers dans** `tools/` (sans encore forcer l’utilisation du CSV C3 dans le code).

- **Livrable**
  - **1 PR** “Step 1 – Docs & scripts” → `main`, revue par les 2 autres.

- **Assigné**
  - Collaborateur·trice **1**.

---

## Étape 2 – Backend C++ : config + drivers + processors (Personne 2)

- **Branche** : `feature/c3-step2-backend`

- **Fichiers principaux**
  - **Config & composants :**
    - `src/config/ConfigCache.{h,cpp}`
    - `src/config/ConfigLoader.cpp`
    - `src/components/ComponentDefinition.h`
    - `src/components/ComponentTypes.h`
    - `src/components/Definitions.h`
    - `src/components/FormFieldHelpers.h`
    - `src/components/MidiMessageFactory.h`
    - `src/components/ValidationRegistry.cpp`
    - `src/components/basic/*Def.*` (Button, Joystick, Led, Potentiometer, Touch, Velostat, …)
    - `src/components/interface/Mpr121Def.*`
    - `src/components/motion/Lis3dhDef.*`
  - **Drivers & hardware :**
    - `src/hardware/I2CManager.{h,cpp}`
    - `src/hardware/Lis3dhDriver.{h,cpp}`
    - `src/hardware/Mpr121Driver.{h,cpp}`
  - **Processors & managers :**
    - `src/processors/*` (Button/Imu/Joystick/Mpr121/Potentiometer/Touch/Velostat/Ultrasonic…)
    - `src/managers/ComponentManager.{h,cpp}`
    - `src/managers/MuxManager.{h,cpp}`
    - `src/managers/complex/joystick/*`
    - `src/midi/MidiMessageType.cpp`
    - `src/midi/handlers/MidiMessageHandler.h`
  - **Intégration serveur :**
    - `src/NiDMI.cpp`
    - `src/api/ComponentsAPI.cpp`
    - `src/api/NetworkAPI.cpp`
    - `src/api/PinAPI.cpp`
    - `src/api/RTPAPI.cpp`
    - `src/server/ServerCallbacks.h`
    - `src/utils/ComponentInitializer.cpp`
    - `src/utils/AnalogFilter.h`
    - `src/utils/JSONParser.cpp`

- **Tâches**
  - **Importer ces changements en blocs cohérents** (par ex. d’abord drivers I2C, puis nouveaux processors, puis adaptations `ComponentManager`, etc.).
  - **Vérifier que le backend compile sur C3 et S3** avec les cores cibles.
  - **Ajouter si besoin des `#ifdef`** pour que `main` reste compatible avec les anciens boards pendant la transition.

- **Livrable**
  - **1 PR** “Step 2 – Backend config + drivers + processors” → `main`.

- **Assigné**
  - Collaborateur·trice **2**.

---

## Étape 3 – UI & front web (Personne 3)

- **Branche** : `feature/c3-step3-ui-web`

- **Fichiers principaux**
  - **Côté C++ UI :**
    - `src/ui/ui_bundle.h`
    - `src/ui/ui_index.cpp`
    - `src/ui/WebAPI.cpp`
  - **Côté web :**
    - `web/index.html`
    - `build/index.min.html`
  - **Tout le JS :**
    - `web/js/api.js`
    - `web/js/component-config.js`
    - `web/js/component-form.js`
    - `web/js/component-handlers.js`
    - `web/js/component-helpers.js`
    - `web/js/component-menus.js`
    - `web/js/definitions.js`
    - `web/js/form-generator.js`
    - `web/js/gpio-manager.js`
    - `web/js/midi-config.js`
    - `web/js/pin-list.js`
    - `web/js/pin-utils.js`
    - `web/js/pin-visual.js`
    - `web/js/system-config.js`
    - `web/js/websocket.js`

- **Tâches**
  - **Reprendre la nouvelle UI et les scripts JS** de la feature, mais en gardant une **compatibilité avec les données/JSON actuels du backend** (Étape 2).
  - **S’assurer que les nouveaux écrans/options liés aux partitions C3/no-spiffs** sont visibles mais, si besoin, **désactivables par configuration** (pour ne pas surprendre les utilisateurs actuels).
  - **Vérifier la synchro** entre `ui_bundle.h` et les fichiers `web/` générateurs.

- **Livrable**
  - **1 PR** “Step 3 – UI & web” → `main`.

- **Assigné**
  - Collaborateur·trice **3**.

---

## Étape 4 – Partitions C3 / no SPIFFS (intégration finale)

- **Branche** : `feature/c3-step4-partitions`

- **Fichiers principaux**
  - `tools/nidmi_c3_no_spiffs.csv`
  - Éventuelles références dans :
    - `scripts/nidmi.sh`
    - `docs/*`
    - `web/js/system-config.js`
    - autres fichiers liés au build / flash

- **Tâches**
  - **Définir clairement :**
    - **quelles cibles** utilisent ce layout (C3 uniquement ?),
    - **comment l’utilisateur choisit ce profil** (via `nidmi.sh`, via l’IDE, doc).
  - **Tester le flash et le fonctionnement complet** sur **ESP32-C3** avec ce layout.
  - **Documenter les limitations** (plus de SPIFFS, etc.) dans les `docs/`.

- **Livrable**
  - **1 PR** “Step 4 – Partitions C3 / no SPIFFS” → `main`.

- **Assigné**
  - À décider (par ex. la personne la plus à l’aise avec les outils de build / flash).

---

## Organisation d’équipe

- **Pour chaque step**
  - **1 branche dédiée** depuis `main`.
  - **1 personne “driver”** de la step, les 2 autres comme **reviewers**.
  - Toujours :
    - `git fetch && git rebase main` avant d’ouvrir/mettre à jour une PR.
    - **Vérifier compilation et tests de base (C3/S3)** avant merge.

- **Communication**
  - Créer un **milestone GitHub** “Migration C3 no SPIFFS”.
  - Créer **4 issues** : “Step 1… Step 4…”, chacune assignée.
  - Optionnel : un petit **project board** (To do / In progress / In review / Done).