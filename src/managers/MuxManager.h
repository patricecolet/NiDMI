#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "../hardware/AnalogMux.h"
#include "../utils/Hysteresis.h"
#include "../hardware/MuxConstants.h"

class MidiSender;
class OSCQueue;

// Format OSC pour multiplexeur
enum class MuxOSCFormat {
    RAW = 0,    // Données brutes (0-4095) comme uint16_t
    FLOAT = 1,  // Normalisé (0-1) comme float
    MIDI = 2    // MIDI standard (0-127) comme uint8_t
};

// Configuration d'un multiplexeur analogique
struct MuxConfig {
    uint8_t sig_pin;       // Pin analogique (SIG)
    uint8_t s0, s1, s2, s3; // Pins de selection
    uint8_t en_pin;        // Pin enable (255 = non connectee)
    bool enabled;          // Multiplexeur actif
    uint8_t cc_base;       // CC de base pour le MIDI (canal 0 → cc_base)
    uint8_t midi_channel;  // Canal MIDI pour tous les canaux MUX
    uint16_t analog_min[16];  // Seuil minimum par canal (0-4095, défaut: 0)
    uint16_t analog_max[16];  // Seuil maximum par canal (0-4095, défaut: 4095)
    bool hysteresis_enabled;  // Hystérésis activée (défaut: true)
    MuxOSCFormat osc_format;  // Format OSC (défaut: FLOAT)
    uint8_t filter_intensity; // Intensité du filtrage (1-10): 1=rapide, 10=stable (défaut: 5)
    char osc_base[64];        // Adresse OSC de base (défaut: /mux{id})
    
    MuxConfig() : sig_pin(0), s0(0), s1(0), s2(0), s3(0), en_pin(255),
                  enabled(false), cc_base(1), midi_channel(1),
                  hysteresis_enabled(true), osc_format(MuxOSCFormat::FLOAT), filter_intensity(5) {
        for (uint8_t i = 0; i < 16; i++) {
            analog_min[i] = 0;
            analog_max[i] = 4095;
        }
        osc_base[0] = '\0';
    }
};

class MuxManager {
public:
    MuxManager();
    ~MuxManager();
    
    bool addMux(uint8_t mux_id, uint8_t sig, uint8_t s0, uint8_t s1, uint8_t s2, uint8_t s3,
                uint8_t en, uint16_t analog_min, uint16_t analog_max,
                bool hysteresis_enabled, MuxOSCFormat osc_format, uint8_t filter_intensity,
                uint8_t cc_base, uint8_t midi_channel, const char* osc_base);
    bool removeMux(uint8_t mux_id);
    
    uint8_t getMuxCount() const { return mux_count; }
    const MuxConfig* getMuxConfig(uint8_t mux_id) const;
    
    bool isMuxGpio(uint8_t gpio) const {
        return gpio >= MUX_GPIO_BASE && gpio < MUX_GPIO_BASE + MAX_MUXES * MUX_CHANNELS;
    }
    
    void updateMuxCache(uint8_t mux_id);
    void updateAllCaches();
    void sendOscBatches(OSCQueue& osc_queue);
    void sendMidiUpdates(MidiSender* midi_sender);
    
    // Gestion de la tâche FreeRTOS
    void begin();
    void stop();
    TaskHandle_t getTaskHandle() const { return muxTaskHandle; }
    
    bool readMuxAllChannels(uint8_t mux_id, uint16_t* values);
    uint16_t readMuxChannel(uint8_t gpio);
    
    bool calibrateMux(uint8_t mux_id, uint8_t channel, bool is_min, bool all_channels, OSCQueue& osc_queue);
    bool resetMuxThresholds(uint8_t mux_id, uint8_t channel, bool all_channels, OSCQueue& osc_queue);
    
    void loadMuxConfigFromNVS();
    
private:
    // Fonction de mapping : 1-10 vers alpha 0.5-0.05 (inversé)
    static float mapFilterIntensity(uint8_t intensity);
    
    struct MuxChannelFilter {
        float alpha;
        float filtered;
        bool initialized;
        
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
        
        void reset() { initialized = false; }
    };
    
    struct MuxCache {
        uint16_t raw_values[16];      // Valeurs brutes (0-4095)
        uint16_t filtered_values[16]; // Valeurs filtrées (0-4095)
        uint8_t stable_values[16];    // Valeurs stables après hystérésis (0-127)
        uint8_t last_sent_values[16]; // Dernières valeurs envoyées en MIDI
        MuxChannelFilter filters[16]; // Filtres par canal
        Hysteresis<2> hysteresis[16]; // Hystérésis par canal
        uint32_t last_update;         // Dernière mise à jour
        bool valid;                    // Cache valide
        bool values_changed;           // Flag pour détecter les changements
        
        MuxCache() : last_update(0), valid(false), values_changed(false) {
            for (int i = 0; i < 16; i++) {
                filters[i].alpha = 0.1f;
                filters[i].initialized = false;
                hysteresis[i].prevLevel = 0;
                last_sent_values[i] = 255;
            }
        }
    };
    
    MuxConfig mux_configs[MAX_MUXES];
    AnalogMux* muxes[MAX_MUXES];
    MuxCache mux_cache[MAX_MUXES];
    uint8_t mux_count;
    
    uint8_t computeCcForChannel(const MuxConfig& config, uint8_t channel) const;
    
    // FreeRTOS task pour lecture multiplexeurs sur Core 0
    TaskHandle_t muxTaskHandle;
    bool taskStarted;
    static void muxTask(void* parameter);
    void muxTaskLoop();
};
