#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "PinMapper.h"
#include "midi/MidiSender.h"

// Types de composants supportés
enum class ComponentType : uint8_t {
    POTENTIOMETER = 0,
    BUTTON = 1,
    LED = 2
};

// Configuration optimisée d'un composant (8 bytes)
struct ComponentConfig {
    uint8_t gpio;           // Pin GPIO
    ComponentType type;     // Type de composant
    uint8_t midi_param;    // CC/Note/Program number
    uint8_t midi_channel;  // Canal MIDI (1-16)
    uint8_t flags;         // Flags (rtp_enabled, etc.)
    uint8_t reserved[3];   // Padding pour alignement
};

// État runtime d'un composant (12 bytes)
struct ComponentState {
    uint16_t last_value;    // Dernière valeur lue
    uint32_t last_time;     // Dernière mise à jour
    uint8_t debounce_state; // État anti-rebond
    uint8_t reserved;       // Padding
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
    
    // Filtre analogique optimisé (selon ARCHITECTURE_MIDI.md)
    struct AnalogFilter {
        float alpha;
        float filtered;
        bool initialized;
        
        uint16_t process(uint16_t raw) {
            if (!initialized) { 
                filtered = raw; 
                initialized = true; 
                return raw; 
            }
            filtered = alpha * raw + (1.0f - alpha) * filtered;
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
    
    // Gestion des composants
    bool addComponent(uint8_t gpio, ComponentType type, uint8_t midi_param, uint8_t channel);
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
