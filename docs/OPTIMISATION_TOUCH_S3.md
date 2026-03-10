## Ce document décrit des optimisations S3 en partie implémentées, en partie à venir.
## État : certaines optimisations encore à faire
- Option : rendre la taille de pas configurable (fine vs “économe en messages”).

**Bénéfice** :

- Moins de changements de valeur = moins de messages MIDI/OSC,
- Le contrôle reste fluide mais plus “stable”.

---

### O4 – Protections côté `OSCQueue` / serveur

**Problème** :

- En config extrême (beaucoup de sources, pas seulement Touch), `OSCQueue` peut se remplir plus vite que le serveur ne vide.

**Idée** :

- Définir une **taille max** pour la queue :
  - si pleine, dropper silencieusement les nouveaux messages,
  - ou compacter (remplacer une série de valeurs par la dernière).
- Ajouter un **throttling global OSC** (par ex. max X messages/s), appliqué par `OSCQueue` avant de pousser vers le réseau.

**Bénéfice** :

- Évite que le serveur / WiFi se retrouve saturé par un flux OSC excessif.
- Protège contre les configs “mauvaises” côté utilisateur sans tout casser.

---

### O5 – Scheduler par type de composant

**Problème** :

- Tous les composants sont traités au même rythme et avec la même priorité dans `MidiTaskLoop`, alors que certains tolèrent plus de latence.

**Idée** :

- Donner un **rythme différent par type** :

  - Boutons / événements discrets : **chaque cycle**.
  - Touch / potards / IMU : **une fois tous les N cycles** (par ex. N=2 ou 3).
  - MUX : éventuellement géré déjà en tâche séparée (`MuxTask`), mais rendre explicite la fréquence de mise à jour.

- Implémenter ça dans `MidiTaskLoop` via des compteurs simples par type ou par index.

**Bénéfice** :

- Le CPU est utilisé en priorité pour les événements “critiques” (boutons, clocks, etc.).
- Les capteurs continus restent fluides tout en consommant moins de temps.

---

### O6 – Outils de diagnostic de charge

**Problème** :

- Difficile de savoir quand on s’approche à nouveau du WDT ou d’un point de saturation.

**Idée** :

- En mode debug (ou via un flag `DEBUG_PERF`):
  - mesurer le temps de chaque cycle `MidiTaskLoop` (`millis` avant/après),
  - loguer périodiquement :
    - temps moyen / max sur une fenêtre,
    - `component_count`, nombre de Touch, nombre de MUX actifs.
- Eventuellement exposer ces infos via un endpoint ou l’UI (page de debug).

**Bénéfice** :

- Permet de valider objectivement les effets des optimisations.
- Aide à diagnostiquer les futures configs “extrêmes” sans tâtonner.

---

## 🧾 Résumé rapide

- **Déjà fait** : round‑robin dans `MidiTask` → plus de WDT avec 7 pins.
- **À faire plus tard** :
  - O1 : Sampler global Touch (round‑robin GPIO) pour réduire les `touchRead`.
  - O2 : Ajuster / configurer la fréquence des updates MIDI/OSC en continu.
  - O3 : Quantifier les valeurs pour limiter les micro‑variations.
  - O4 : Limiter / protéger `OSCQueue`.
  - O5 : Scheduler par type de composant.
  - O6 : Outils de diagnostic de charge (logs perf).

Ces optimisations permettront de monter sereinement en nombre de pads touch + autres capteurs, sans risquer de retomber sur le watchdog ou de saturer le serveur.