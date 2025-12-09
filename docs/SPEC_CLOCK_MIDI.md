# Spécification - Système de Clock MIDI

## Vue d'ensemble

Ce document décrit la spécification pour un système de génération et synchronisation de MIDI Clock sur l'ESP32, avec plusieurs sources d'entrée et modes de fonctionnement.

**Date de création :** 2024  
**Statut :** Spécification (à implémenter)

---

## Objectifs

Créer un système flexible de génération de MIDI Clock qui permet :
1. ✅ Génération autonome de clock avec ajustement manuel
2. ✅ Tap Tempo pour caler le tempo manuellement
3. ✅ Synchronisation externe (MIDI Clock entrant)
4. ✅ Contrôle ON/OFF de la clock

---

## Fonctionnalités principales

### 1. Toggle ON/OFF de la Clock

**Description :** Un bouton permet d'activer/désactiver la génération de MIDI Clock.

**Comportement :**
- **État OFF** : Aucune clock n'est générée, même si d'autres paramètres sont configurés
- **État ON** : La clock est générée selon la source active (interne, tap tempo, ou sync externe)
- Le toggle peut être un bouton physique ou une commande depuis l'interface web

**Interface :**
- Bouton web : Checkbox "Activer Clock MIDI"
- État visible : Indicateur LED ou feedback visuel dans l'interface

---

### 2. Tap Tempo

**Description :** Mesure des intervalles entre les taps et calcul automatique du BPM pour générer une clock continue.

**Comportement :**
- **Premier tap** : Enregistre le timestamp, attend le 2ème tap
- **2ème tap** : Calcule le BPM initial basé sur l'intervalle, démarre la clock
- **Taps suivants** : Recalcule le BPM basé sur la moyenne des 2-4 derniers intervalles
- **Timeout** : Si aucun tap pendant 3 secondes, la clock continue au dernier BPM calculé
- **Arrêt** : Si le toggle est OFF, la clock s'arrête même si des taps arrivent

**Calcul du BPM :**
```
BPM = 60000 / intervalle_moyen_ms

Avec :
- Intervalle moyen = moyenne des 2-4 derniers intervalles
- Minimum 2 taps requis pour démarrer
- Maximum 4 intervalles pour la moyenne (fenêtre glissante)
```

**Interface :**
- Bouton physique ou web : "Tap Tempo"
- Affichage : BPM actuel dans l'interface web (ex: "120 BPM")

---

### 3. Potentiomètre pour ajustement du Tempo

**Description :** Un potentiomètre permet d'ajuster finement le BPM de la clock générée.

**Comportement :**
- **Plage de BPM** : 20-300 BPM (configurable)
- **Mapping** : 0-4095 (ADC 12 bits) → 20-300 BPM
- **Priorité** : 
  - Si Tap Tempo est actif récemment (< 3s), le potard ajuste le BPM de base
  - Sinon, le potard définit directement le BPM
- **Filtrage** : Filtre passe-bas pour éviter les variations brusques

**Interface :**
- Potentiomètre physique : Pin ADC configuré comme "Tempo Adjust"
- Affichage : BPM dans l'interface web, mis à jour en temps réel

**Formule :**
```
BPM_pot = map(adc_value, 0, 4095, 20, 300)
BPM_final = moyenne(BPM_tap, BPM_pot)  // Si tap actif
BPM_final = BPM_pot  // Si tap inactif
```

---

### 4. Synchronisation externe (MIDI Clock entrant)

**Description :** Le système peut recevoir une MIDI Clock externe et s'y synchroniser.

**Comportement :**
- **Détection** : Si des messages MIDI Clock arrivent sur un canal configuré
- **Mode Sync** : 
  - Le système calcule le BPM de la clock entrante
  - Génère une clock synchronisée (phase-locked) à la source externe
- **Priorité** : La sync externe prend le dessus sur Tap Tempo et Potentiomètre
- **Timeout** : Si aucun Clock entrant pendant 2 secondes, bascule vers mode interne (Tap/Pot)

**Détection du BPM externe :**
```
- Compter les ticks MIDI Clock reçus sur une fenêtre de 1 seconde
- BPM_externe = ticks_count * (60 / 24)  // 24 ticks par noire
- Ou : mesurer l'intervalle entre 2 ticks et calculer
```

**Interface :**
- Configuration : Canal MIDI d'entrée pour la sync
- Indicateur : LED ou feedback "Sync Externe Active"
- Affichage : BPM externe détecté dans l'interface web

---

## Architecture technique

### Composants nécessaires

1. **ClockGenerator** (nouvelle classe)
   - Gestion de l'état (ON/OFF)
   - Calcul du BPM (Tap Tempo, Potentiomètre, Sync externe)
   - Génération périodique de MIDI Clock ticks

2. **TapTempoCalculator**
   - Stockage des timestamps des taps
   - Calcul de la moyenne des intervalles
   - Gestion du timeout

3. **MidiClockReceiver** (extension)
   - Réception des messages MIDI Clock entrant
   - Calcul du BPM externe
   - Détection de perte de signal

4. **TempoPotentiometer**
   - Lecture ADC avec filtrage
   - Mapping vers BPM

### États du système

```
État OFF → Clock désactivée (aucune génération)

État ON → Mode actif choisi :
  ├─ Mode INTERNE (Tap Tempo ou Potentiomètre)
  │  ├─ Tap Tempo actif (< 3s) → BPM = f(Tap, Pot)
  │  └─ Tap Tempo inactif → BPM = f(Pot)
  │
  └─ Mode SYNC EXTERNE
     └─ Clock externe détectée → Génération synchronisée
```

### Priorités

1. **Toggle OFF** : Aucune clock générée
2. **Sync Externe active** : Génération synchronisée (priorité haute)
3. **Tap Tempo actif** : Génération basée sur Tap + ajustement Pot
4. **Potentiomètre seul** : Génération basée uniquement sur Pot

---

## Interface utilisateur

### Interface web

**Section "Clock MIDI"** :
```
┌─────────────────────────────────────┐
│ 🎵 Clock MIDI                       │
├─────────────────────────────────────┤
│ [✓] Activer Clock MIDI              │
│                                     │
│ Mode actif : [Tap Tempo]            │
│ BPM actuel : 120                    │
│                                     │
│ Sources :                           │
│  • Tap Tempo : Actif (3s ago)       │
│  • Potentiomètre : 118 BPM          │
│  • Sync externe : Inactif           │
└─────────────────────────────────────┘
```

**Configuration des pins** :
- Bouton : Mode "Toggle Clock" ou "Tap Tempo"
- Potentiomètre : Mode "Tempo Adjust"
- Affichage du BPM en temps réel pour chaque source

---

## Implémentation technique

### Timing de la Clock MIDI

**Spécification MIDI :**
- 24 ticks par noire (quarter note)
- Intervalle entre ticks = `(60000 / BPM) / 24` ms
- Exemple : 120 BPM → `2500 / 120` = 20.83 ms entre ticks

**Génération périodique :**
```cpp
// Pseudo-code
void ClockGenerator::update() {
    if (!enabled) return;
    
    uint32_t now = millis();
    uint32_t interval = calculateInterval(); // Basé sur BPM actuel
    
    if (now - lastTickTime >= interval) {
        sendClock();
        lastTickTime = now;
    }
}
```

### Structure de données

```cpp
struct ClockState {
    bool enabled;              // Toggle ON/OFF
    uint16_t currentBPM;       // BPM actuel (20-300)
    ClockSource activeSource;  // INTERNAL, TAP, SYNC_EXTERNAL
    
    // Tap Tempo
    uint32_t tapTimestamps[4]; // Derniers 4 taps
    uint8_t tapCount;
    uint32_t lastTapTime;
    bool tapActive;            // True si tap récent (< 3s)
    
    // Potentiomètre
    uint16_t potBPM;           // BPM du potentiomètre
    uint16_t lastPotValue;     // Valeur ADC filtrée
    
    // Sync externe
    bool syncExternalActive;
    uint16_t externalBPM;
    uint32_t lastExternalClock;
    
    // Génération
    uint32_t lastTickTime;     // Dernier tick généré
    uint32_t tickInterval;     // Intervalle calculé en ms
};
```

---

## Exemples d'utilisation

### Cas 1 : Clock manuelle avec potentiomètre
1. Activer le toggle "Clock MIDI ON"
2. Tourner le potentiomètre pour ajuster le BPM (ex: 120 BPM)
3. La clock génère 24 ticks/seconde à 120 BPM

### Cas 2 : Tap Tempo
1. Activer le toggle "Clock MIDI ON"
2. Taper 4 fois sur le bouton Tap Tempo à intervalles réguliers
3. Le système calcule le BPM moyen (ex: 115 BPM)
4. La clock génère automatiquement à ce BPM
5. Le potentiomètre peut affiner le BPM si nécessaire

### Cas 3 : Synchronisation externe
1. Activer le toggle "Clock MIDI ON"
2. Connecter une source externe qui envoie MIDI Clock
3. Le système détecte automatiquement et se synchronise
4. La clock générée suit la source externe

### Cas 4 : Mixte (Tap + Sync)
1. Clock active avec Tap Tempo (120 BPM)
2. Une source externe commence à envoyer MIDI Clock (125 BPM)
3. Le système bascule automatiquement vers la sync externe
4. Si la source externe s'arrête (> 2s), retour vers Tap Tempo

---

## Paramètres configurables

- **Plage BPM** : Min/Max (défaut: 20-300)
- **Fenêtre Tap Tempo** : Nombre d'intervalles pour moyenne (2-4, défaut: 3)
- **Timeout Tap Tempo** : Temps avant désactivation (défaut: 3000ms)
- **Timeout Sync externe** : Temps avant retour mode interne (défaut: 2000ms)
- **Filtre potentiomètre** : Coefficient passe-bas (défaut: 0.1)

---

## Tests et validation

### Scénarios de test

1. **Toggle ON/OFF** : Vérifier que la clock s'arrête/démarre
2. **Tap Tempo** : Vérifier calcul correct du BPM sur différents rythmes
3. **Potentiomètre** : Vérifier mapping linéaire et filtrage
4. **Sync externe** : Vérifier détection et synchronisation
5. **Priorités** : Vérifier l'ordre de priorité des sources
6. **Timeouts** : Vérifier les bascules automatiques après timeout

### Métriques

- **Précision BPM** : ±1 BPM
- **Jitter clock** : < 1ms (sur ESP32-S3 avec FreeRTOS)
- **Latence Tap Tempo** : < 50ms pour démarrer après 2ème tap

---

## Notes d'implémentation

### FreeRTOS

Pour la génération périodique de clock, deux approches possibles :
1. **Task dédiée** : Task FreeRTOS avec `vTaskDelayUntil()` pour timing précis
2. **Polling dans loop()** : Vérification dans `ComponentManager::update()`

**Recommandation** : Task dédiée pour meilleure précision temporelle.

### Ressources

- **RAM** : ~200 bytes pour `ClockState`
- **CPU** : < 1% (génération à 24 ticks/seconde max = 300 BPM)
- **Timers** : Optionnel, peut utiliser `millis()` pour timing

---

## Évolution future

### Fonctionnalités potentielles

- **Divisions de tempo** : 1/4, 1/8, 1/16, etc.
- **Start/Stop/Continue** : Contrôle complet de la clock
- **Song Position Pointer (SPP)** : Synchronisation avec position dans la chanson
- **Multi-sorties** : Générer clock sur plusieurs canaux MIDI
- **Enregistrement de patterns** : Sauvegarder des rythmes tapés

---

## Références

- MIDI 1.0 Specification : MIDI Clock = 24 ticks per quarter note
- ESP32 FreeRTOS Documentation
- Control Surface Library (inspiration pour Tap Tempo)

---

**Document à compléter lors de l'implémentation.**

