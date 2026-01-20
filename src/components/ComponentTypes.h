#pragma once

#include <Arduino.h>
#include "../midi/MidiMessageType.h"
#include "../utils/Hysteresis.h"

/**
 * @file ComponentTypes.h
 * @brief Définitions communes pour les composants
 * 
 * Ce fichier centralise les types et structures utilisés par tous les processeurs
 * et le ComponentManager. Cela évite les dépendances circulaires et facilite
 * l'ajout de nouveaux types de composants.
 */

// Types de composants supportés
enum class ComponentType : uint8_t {
    POTENTIOMETER = 0,
    BUTTON        = 1,
    LED           = 2,
    MUX           = 3,
    VELOSTAT      = 4,
    ULTRASONIC    = 5
    // Facilement extensible pour de nouveaux types
    // Exemple: ENCODER = 6, FADER = 7, etc.
};

// Types de pins supportés par un composant
enum class PinType : uint8_t {
    PIN_ANALOG = 0,          // Pin analogique uniquement (ADC)
    PIN_DIGITAL = 1,         // Pin digitale uniquement
    PIN_ANALOG_OR_DIGITAL = 2, // Les deux sont acceptés
    PIN_PWM = 3              // Pin avec capacité PWM
};

// Configuration optimisée d'un composant
struct ComponentConfig {
    uint8_t gpio;           // Pin GPIO
    ComponentType type;     // Type de composant
    uint8_t midi_param;    // CC/Note/Program number
    uint8_t midi_channel;  // Canal MIDI (1-16)
    MidiMessageType msg_type; // Type de message MIDI
    uint8_t flags;         // Flags (rtp_enabled, etc.)
    char osc_address[32];  // Adresse OSC par pin (ex: /ctl, /note, /led)
    uint8_t rtpNoteMin;    // Note min pour balayage (NOTE_SWEEP)
    uint8_t rtpNoteMax;   // Note max pour balayage (NOTE_SWEEP)
    uint8_t rtpNoteVelFix; // Vélocité fixe pour balayage (NOTE_SWEEP)
    uint16_t rtpNoteSweepAutoOffDelay; // Délai auto-off en ms (0 = désactivé, max 65535)
    uint8_t midiCcRangeMin; // Plage MIDI min (0-127, défaut: 0) pour CC/autres messages
    uint8_t midiCcRangeMax; // Plage MIDI max (0-127, défaut: 127) pour CC/autres messages
    uint8_t midiCcOnOffMin; // Valeur CC pour état OFF (0-127, défaut: 0) - pour boutons
    uint8_t midiCcOnOffMax; // Valeur CC pour état ON (0-127, défaut: 127) - pour boutons
    
    // Champs spécifiques existants (pour compatibilité/rétrocompatibilité)
    char btnMode[16];     // Mode bouton: "pulse", "press_release", "toggle"
    char btnPulseTiming[16]; // Timing pour mode pulse: "press" ou "release"
    char btnPullMode[16]; // Mode pull bouton: "pullup", "pulldown", "none" (défaut: "pullup")
    char ledMode[16];     // Mode LED: "onoff", "pwm"
    uint8_t filter_intensity; // Intensité du filtrage (1-10): 1=rapide, 10=stable (défaut: 5)
    uint16_t potMin;          // Seuil minimum pour potentiomètre (0-4095, défaut: 0)
    uint16_t potMax;          // Seuil maximum pour potentiomètre (0-4095, défaut: 4095)
    
    // Champs génériques pour nouveaux composants (extensible sans modifier la structure)
    char customField1[16];  // Champ générique réutilisable (ex: encoderDirection, touchThreshold, etc.)
    char customField2[16];  // Champ générique réutilisable (ex: encoderMode, touchMode, etc.)
    uint8_t customInt1;     // Valeur numérique générique (ex: encoderSteps, touchSensitivity, etc.)
    uint8_t customInt2;     // Valeur numérique générique supplémentaire
};

// État runtime d'un composant
struct ComponentState {
    uint16_t last_value;    // Dernière valeur lue
    uint32_t last_time;     // Dernière mise à jour
    uint8_t debounce_state; // État anti-rebond
    uint8_t last_note;      // Dernière note jouée (pour NOTE_SWEEP)
    
    // Champs pour debouncing simple et fiable
    bool last_button_state; // État précédent du bouton (avant debounce)
    uint32_t last_change_time; // Temps du dernier changement
    uint32_t note_on_time; // Temps où la note a été jouée (pour auto-off)
    bool toggle_state;     // État pour mode toggle (true = note on, false = note off)
    bool prev_stable_state; // État stable précédent (après debounce) pour détecter Falling/Rising
    bool pulse_pending;    // Pour pulse: mémoriser qu'on a été pressé, attendre release
    
    // Hystérésis pour NOTE_SWEEP (zone morte de 2 bits = ±3 sur 0-127)
    Hysteresis<2> hysteresis;
    
    // Pour Velostat : dernière valeur aftertouch envoyée
    uint8_t last_aftertouch;
};
