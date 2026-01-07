#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "PinMapper.h"
#include "midi/MidiSender.h"
#include "midi/MidiMessageType.h"
#include "OSCManager.h"
#include "OSCQueue.h"
#include "components/AnalogMux.h"

// Types de composants supportés
enum class ComponentType : uint8_t {
    POTENTIOMETER = 0,
    BUTTON = 1,
    LED = 2
};

// Format OSC pour multiplexeur
enum class MuxOSCFormat {
    RAW = 0,    // Données brutes (0-4095) comme uint16_t
    FLOAT = 1,  // Normalisé (0-1) comme float
    MIDI = 2    // MIDI standard (0-127) comme uint8_t
};

// Configuration d'un multiplexeur analogique
struct MuxConfig {
    uint8_t sig_pin;      // Pin analogique (SIG)
    uint8_t s0, s1, s2, s3; // Pins de selection
    uint8_t en_pin;       // Pin enable (255 = non connectee)
    bool enabled;         // Multiplexeur actif
    uint16_t analog_min;  // Seuil minimum (0-4095, défaut: 0)
    uint16_t analog_max;  // Seuil maximum (0-4095, défaut: 4095)
    bool hysteresis_enabled; // Hystérésis activée (défaut: true)
    MuxOSCFormat osc_format; // Format OSC (défaut: FLOAT)
    uint8_t filter_intensity; // Intensité du filtrage (1-10): 1=rapide, 10=stable (défaut: 5)
    
    MuxConfig() : sig_pin(0), s0(0), s1(0), s2(0), s3(0), en_pin(255), 
                  enabled(false), analog_min(0), analog_max(4095), 
                  hysteresis_enabled(true), osc_format(MuxOSCFormat::FLOAT), filter_intensity(5) {}
};

// Constantes pour les GPIO virtuels des multiplexeurs
// GPIO 200-215 = MUX0 canaux 0-15
// GPIO 216-231 = MUX1 canaux 0-15
// Limité à 2 multiplexeurs pour éviter le manque de pins digitales
static constexpr uint8_t MUX_GPIO_BASE = 200;
static constexpr uint8_t MUX_CHANNELS = 16;
static constexpr uint8_t MAX_MUXES = 2;

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
    uint8_t filter_intensity; // Intensité du filtrage (1-10): 1=rapide, 10=stable (défaut: 5)
};

// Hystérésis unifiée pour tous les composants - méthode Control-Surface
// Réduit directement la résolution de 12 bits (0-4095) vers 7 bits (0-127)
// BITS = nombre de bits de "zone morte" pour l'hystérésis (typiquement 2)
// Plus BITS est grand, plus la zone morte est large
template <uint8_t BITS>
struct Hysteresis {
    uint8_t prevLevel = 0;  // Stocke directement 0-127
    
    // Retourne true si la valeur a changé (après hystérésis)
    // Prend une valeur haute résolution (0-4095) et la réduit vers 0-127
    bool update(uint16_t input) {
        // Constantes pour la zone morte (comme Control-Surface)
        constexpr uint16_t margin = (1ul << BITS) - 1ul;  // Ex: BITS=2 -> margin=3
        constexpr uint16_t offset = BITS >= 1 ? (1ul << (BITS - 1)) : 0;
        constexpr uint16_t max_in = 4095;  // Valeur max d'entrée (12 bits)
        constexpr uint8_t max_out = 127;   // Valeur max de sortie (7 bits)
        
        // Remettre prevLevel (0-127) en haute résolution (0-4095) pour les calculs
        // On utilise >> 5 pour réduire, donc << 5 pour remettre en haute résolution
        uint16_t prevLevelFull = ((uint16_t)prevLevel << 5) | offset;
        
        // Calculer les bornes sur la valeur haute résolution (uint16_t)
        uint16_t lowerbound = prevLevel > 0 ? 
            (prevLevelFull > margin ? prevLevelFull - margin : 0) : 0;
        uint16_t upperbound = prevLevel < max_out ? 
            (prevLevelFull + margin > max_in ? max_in : prevLevelFull + margin) : max_in;
        
        // Comparer avec la valeur haute résolution d'entrée (comme Control-Surface)
        if (input < lowerbound || input > upperbound) {
            // Réduire la résolution seulement maintenant : 12 bits → 7 bits (>> 5)
            prevLevel = input >> 5;
            return true;  // Valeur a changé
        }
        return false;  // Pas de changement
    }
    
    uint8_t getValue() const { 
        // Retourne directement 0-127 (méthode Control-Surface)
        return prevLevel;
    }
    
    void reset(uint16_t value) {
        prevLevel = value >> 5;  // 12 bits → 7 bits
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
    /* MAX_COMPONENTS doit supporter 2 MUX (32 pins) + autres composants */
    static constexpr uint8_t MAX_COMPONENTS = 64;
    
    ComponentConfig configs[MAX_COMPONENTS];
    ComponentState states[MAX_COMPONENTS];
    uint8_t component_count;
    MidiSender* midi_sender;
    OSCManager osc_manager;
    OSCQueue osc_queue;
    
    // Multiplexeurs analogiques
    MuxConfig mux_configs[MAX_MUXES];
    AnalogMux* muxes[MAX_MUXES];  // Pointeurs (nullptr si non configure)
    uint8_t mux_count;
    
    // Fonction de mapping : 1-10 vers alpha 0.5-0.05 (inversé)
    // 1 → alpha = 0.5 (filtrage minimum, réponse rapide)
    // 10 → alpha = 0.05 (filtrage maximum, réponse lente)
    static float mapFilterIntensity(uint8_t intensity) {
        // Clamp entre 1 et 10
        if (intensity < 1) intensity = 1;
        if (intensity > 10) intensity = 10;
        
        // Mapping inverse : plus la valeur est élevée, plus alpha est faible
        // Utiliser une courbe quadratique pour plus de contrôle dans la plage utile
        float normalized = (intensity - 1) / 9.0f; // 0.0 (valeur=1) à 1.0 (valeur=10)
        
        // Courbe quadratique inversée pour plus de contrôle dans la plage haute (filtrage max)
        float alpha_min = 0.05f;  // Filtrage maximum (valeur=10)
        float alpha_max = 0.5f;   // Filtrage minimum (valeur=1)
        
        // Inverser : normalized=0 → alpha_max, normalized=1 → alpha_min
        float alpha = alpha_max - (alpha_max - alpha_min) * (normalized * normalized);
        
        return alpha;
    }
    
    // Filtre analogique optimisé (selon ARCHITECTURE_MIDI.md)
    struct AnalogFilter {
        float alpha;
        float filtered;
        bool initialized;
        
        // Pour le filtre médian (NOTE_SWEEP uniquement)
        uint16_t median_buffer[5];  // Buffer circulaire pour médian
        uint8_t median_index;      // Index actuel dans le buffer
        bool median_initialized;    // Si le buffer médian est rempli
        
        // Définir alpha depuis filter_intensity (1-10)
        void setAlphaFromIntensity(uint8_t intensity) {
            alpha = mapFilterIntensity(intensity);
        }
        
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
        
        // Note: adaptFilter() supprimé - on utilise maintenant filter_intensity configuré par l'utilisateur
    };
    
    AnalogFilter filters[MAX_COMPONENTS];
    
    // Filtres analogiques par canal MUX (inspiré de FilteredAnalog)
    struct MuxChannelFilter {
        float alpha;
        float filtered;
        bool initialized;
        
        // Définir alpha depuis filter_intensity (1-10)
        void setAlphaFromIntensity(uint8_t intensity) {
            alpha = mapFilterIntensity(intensity);
        }
        
        uint16_t process(uint16_t raw) {
            if (!initialized) {
                filtered = raw;
                initialized = true;
                return raw;
            }
            filtered = alpha * raw + (1.0f - alpha) * filtered;
            return (uint16_t)filtered;
        }
        
    void reset() {
        initialized = false;
    }
};


// Cache des valeurs MUX lues en batch
struct MuxCache {
    uint16_t raw_values[16];      // Valeurs brutes (0-4095)
    uint16_t filtered_values[16]; // Valeurs filtrées (0-4095)
    uint8_t stable_values[16];    // Valeurs stables après hystérésis (0-127) - méthode Control-Surface
    MuxChannelFilter filters[16]; // Filtres par canal
    Hysteresis<2> hysteresis[16]; // Hystérésis par canal (réduit directement vers 7 bits)
    uint32_t last_update;         // Dernière mise à jour
    bool valid;                    // Cache valide
    bool values_changed;           // Flag pour détecter les changements
    
    MuxCache() : last_update(0), valid(false), values_changed(false) {
        for (int i = 0; i < 16; i++) {
            filters[i].alpha = 0.1f;
            filters[i].initialized = false;
            hysteresis[i].prevLevel = 0;
        }
    }
};
    
    MuxCache mux_cache[MAX_MUXES]; // Cache pour chaque MUX
    
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
    
    // Gestion des multiplexeurs
    bool addMux(uint8_t mux_id, uint8_t sig, uint8_t s0, uint8_t s1, uint8_t s2, uint8_t s3, 
                uint8_t en = 255, uint16_t analog_min = 0, uint16_t analog_max = 4095, 
                bool hysteresis_enabled = true, MuxOSCFormat osc_format = MuxOSCFormat::FLOAT,
                uint8_t filter_intensity = 5);
    bool removeMux(uint8_t mux_id);
    uint8_t getMuxCount() const { return mux_count; }
    const MuxConfig* getMuxConfig(uint8_t mux_id) const;
    bool isMuxGpio(uint8_t gpio) const { return gpio >= MUX_GPIO_BASE && gpio < MUX_GPIO_BASE + MAX_MUXES * MUX_CHANNELS; }
    uint16_t readMuxChannel(uint8_t gpio);
    
    // Lecture batch optimisée de tous les canaux d'un MUX
    bool readMuxAllChannels(uint8_t mux_id, uint16_t* values);
    void updateMuxCache(uint8_t mux_id); // Mettre à jour le cache MUX
    
    // Debug
    void printStats();
    
private:
    void processPotentiometer(uint8_t index);
    void processButton(uint8_t index);
    void processLed(uint8_t index);
    
    // Utilitaires
    uint8_t findComponentByGpio(uint8_t gpio) const;
    void loadConfigFromNVS();
    void loadMuxConfigFromNVS();
    void saveConfigToNVS();
    
    // Parsing JSON optimisé
    int extractInt(const String& src, const char* key, int def);
    bool extractBool(const String& src, const char* key, bool def);
    String extractStr(const String& src, const char* key, const String& def);
};
