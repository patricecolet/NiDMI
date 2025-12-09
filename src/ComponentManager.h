#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "PinMapper.h"
#include "midi/MidiSender.h"
#include "midi/MidiMessageType.h"
#include "OSCManager.h"
#include "OSCQueue.h"

// Types de composants supportés
enum class ComponentType : uint8_t {
    POTENTIOMETER = 0,
    BUTTON = 1,
    LED = 2
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
    char btnMode[16];     // Mode bouton: "pulse", "press_release", "toggle"
    char btnPulseTiming[16]; // Timing pour mode pulse: "press" ou "release"
};

// Hystérésis pour éviter les oscillations (inspiré de Control_Surface)
// BITS = nombre de bits de "zone morte"
// Plus BITS est grand, plus la zone morte est large
template <uint8_t BITS>
struct Hysteresis {
    uint8_t prevLevel = 0;
    
    // Retourne true si la valeur a changé (après hystérésis)
    bool update(uint8_t input) {
        constexpr uint8_t margin = (1 << BITS) - 1;  // Ex: BITS=2 -> margin=3
        constexpr uint8_t offset = BITS >= 1 ? (1 << (BITS - 1)) : 0;
        
        uint8_t prevLevelFull = (prevLevel << BITS) | offset;
        uint8_t lowerbound = prevLevel > 0 ? prevLevelFull - margin : 0;
        uint8_t upperbound = prevLevel < (127 >> BITS) ? prevLevelFull + margin : 127;
        
        if (input < lowerbound || input > upperbound) {
            prevLevel = input >> BITS;
            return true;  // Valeur a changé
        }
        return false;  // Pas de changement
    }
    
    uint8_t getValue() const { 
        // Retourne la valeur avec les bits de poids faible au milieu de la zone
        constexpr uint8_t offset = BITS >= 1 ? (1 << (BITS - 1)) : 0;
        return (prevLevel << BITS) | offset;
    }
    
    void reset(uint8_t value) {
        prevLevel = value >> BITS;
    }
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
};

/**
 * @brief Manager des composants avec architecture template optimisée
 * 
 * Implémente l'architecture de ARCHITECTURE_MIDI.md :
 * - Structures compactes (8 bytes config + 12 bytes state)
 * - Filtrage analogique adaptatif
 * - Anti-rebond intelligent
 * - Support multi-MCU via PinMapper
 * - Optimisation mémoire (75% de réduction)
 */
class ComponentManager {
private:
    static constexpr uint8_t MAX_COMPONENTS = 32;
    
    ComponentConfig configs[MAX_COMPONENTS];
    ComponentState states[MAX_COMPONENTS];
    uint8_t component_count;
    MidiSender* midi_sender;
    OSCManager osc_manager;
    OSCQueue osc_queue;
    
    // Filtre analogique optimisé (selon ARCHITECTURE_MIDI.md)
    struct AnalogFilter {
        float alpha;
        float filtered;
        bool initialized;
        
        // Pour le filtre médian (NOTE_SWEEP uniquement)
        uint16_t median_buffer[5];  // Buffer circulaire pour médian
        uint8_t median_index;      // Index actuel dans le buffer
        bool median_initialized;    // Si le buffer médian est rempli
        
        uint16_t process(uint16_t raw) {
            if (!initialized) { 
                filtered = raw; 
                initialized = true;
                // Initialiser le buffer médian
                for (int i = 0; i < 5; i++) {
                    median_buffer[i] = raw;
                }
                median_index = 0;
                median_initialized = true;
                return raw; 
            }
            filtered = alpha * raw + (1.0f - alpha) * filtered;
            return (uint16_t)filtered;
        }
        
        // Filtre médian + passe-bas agressif (pour NOTE_SWEEP)
        uint16_t processMedianAndLowpass(uint16_t raw) {
            if (!initialized) {
                filtered = raw;
                initialized = true;
                for (int i = 0; i < 5; i++) {
                    median_buffer[i] = raw;
                }
                median_index = 0;
                median_initialized = true;
                return raw;
            }
            
            // 1. Ajouter la valeur au buffer médian
            median_buffer[median_index] = raw;
            median_index = (median_index + 1) % 5;
            
            // 2. Calculer la médiane (copier, trier, prendre le milieu)
            uint16_t sorted[5];
            for (int i = 0; i < 5; i++) {
                sorted[i] = median_buffer[i];
            }
            // Tri à bulles simple
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4 - i; j++) {
                    if (sorted[j] > sorted[j + 1]) {
                        uint16_t temp = sorted[j];
                        sorted[j] = sorted[j + 1];
                        sorted[j + 1] = temp;
                    }
                }
            }
            uint16_t median_value = sorted[2]; // Médiane de 5 valeurs
            
            // 3. Passe-bas agressif sur la médiane (alpha très petit)
            float aggressive_alpha = 0.05f; // Plus agressif que le filtre normal
            filtered = aggressive_alpha * median_value + (1.0f - aggressive_alpha) * filtered;
            
            return (uint16_t)filtered;
        }
        
        // Adaptation automatique du coefficient selon la vitesse de changement
        void adaptFilter(uint16_t current_value, uint16_t last_value) {
            float change_rate = abs((int)current_value - (int)last_value);
            
            if(change_rate > 50) {
                alpha = 0.3f;  // Changement rapide : filtre moins agressif
            } else if(change_rate < 10) {
                alpha = 0.05f; // Changement lent : filtre plus agressif
            } else {
                alpha = 0.1f;  // Valeur par défaut
            }
        }
    };
    
    AnalogFilter filters[MAX_COMPONENTS];
    
public:
    ComponentManager();
    ~ComponentManager();
    
    void begin(MidiSender* sender);
    void update();
    void reloadConfigs();
    void syncOSCConfig();
    // Gestion des composants
    bool addComponent(uint8_t gpio, ComponentType type, uint8_t midi_param, uint8_t channel, MidiMessageType msg_type = MidiMessageType::NOTE);
    bool removeComponent(uint8_t gpio);
    void clearAll();
    
    // Réception MIDI pour piloter les LEDs
    void handleMidiNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
    void handleMidiNoteOff(uint8_t channel, uint8_t note, uint8_t velocity);
    void handleMidiControlChange(uint8_t channel, uint8_t control, uint8_t value);
    
    // Getters
    uint8_t getComponentCount() const { return component_count; }
    const ComponentConfig* getConfig(uint8_t index) const;
    const ComponentState* getState(uint8_t index) const;
    
    // Debug
    void printStats();
    
private:
    void processPotentiometer(uint8_t index);
    void processButton(uint8_t index);
    void processLed(uint8_t index);
    
    // Utilitaires
    uint8_t findComponentByGpio(uint8_t gpio) const;
    void loadConfigFromNVS();
    void saveConfigToNVS();
    
    // Parsing JSON optimisé
    int extractInt(const String& src, const char* key, int def);
    bool extractBool(const String& src, const char* key, bool def);
    String extractStr(const String& src, const char* key, const String& def);
};
