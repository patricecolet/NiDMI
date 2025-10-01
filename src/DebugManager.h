#ifndef DEBUGMANAGER_H
#define DEBUGMANAGER_H

#include <Arduino.h>

/**
 * @brief Gestionnaire de debug configurable
 * 
 * Cette classe permet de contrôler le debug depuis le sketch
 * avec des options configurables en runtime.
 */
class DebugManager {
public:
    // Options de debug
    bool network = false;
    bool websocket = false;
    bool api = false;
    bool cache = false;
    bool osc = false;
    bool midi = false;
    bool pins = false;
    bool components = false;
    bool rtpMidi = false;
    
    // Niveau de verbosité
    enum VerbosityLevel {
        NONE = 0,
        ERROR = 1,
        WARNING = 2,
        INFO = 3,
        DEBUG = 4
    };
    
    VerbosityLevel verbosity = INFO;
    
    // Constructeur
    DebugManager() = default;
    
    // Méthodes de configuration
    void enableAll() {
        network = websocket = api = cache = osc = midi = pins = components = rtpMidi = true;
    }
    
    void disableAll() {
        network = websocket = api = cache = osc = midi = pins = components = rtpMidi = false;
    }
    
    void setVerbosity(VerbosityLevel level) {
        verbosity = level;
    }
    
    // Méthodes de debug
    void debugNetwork(const char* format, ...);
    void debugWebSocket(const char* format, ...);
    void debugAPI(const char* format, ...);
    void debugCache(const char* format, ...);
    void debugOSC(const char* format, ...);
    void debugMIDI(const char* format, ...);
    void debugPins(const char* format, ...);
    void debugComponents(const char* format, ...);
    void debugRtpMidi(const char* format, ...);
    
    // Méthodes de niveau
    void error(const char* format, ...);
    void warning(const char* format, ...);
    void info(const char* format, ...);
    void debug(const char* format, ...);
    
private:
    void printLog(const char* prefix, const char* format, va_list args);
    bool shouldLog(VerbosityLevel level) const;
};

// Instance globale (sera initialisée depuis le sketch)
extern DebugManager* g_debug;

// Macros de compatibilité pour faciliter la migration
#define debug_network(fmt, ...) if(g_debug && g_debug->network) g_debug->debugNetwork(fmt, ##__VA_ARGS__)
#define debug_websocket(fmt, ...) if(g_debug && g_debug->websocket) g_debug->debugWebSocket(fmt, ##__VA_ARGS__)
#define debug_api(fmt, ...) if(g_debug && g_debug->api) g_debug->debugAPI(fmt, ##__VA_ARGS__)
#define debug_cache(fmt, ...) if(g_debug && g_debug->cache) g_debug->debugCache(fmt, ##__VA_ARGS__)
#define debug_osc(fmt, ...) if(g_debug && g_debug->osc) g_debug->debugOSC(fmt, ##__VA_ARGS__)
#define debug_midi(fmt, ...) if(g_debug && g_debug->midi) g_debug->debugMIDI(fmt, ##__VA_ARGS__)
#define debug_pins(fmt, ...) if(g_debug && g_debug->pins) g_debug->debugPins(fmt, ##__VA_ARGS__)
#define debug_components(fmt, ...) if(g_debug && g_debug->components) g_debug->debugComponents(fmt, ##__VA_ARGS__)
#define debug_rtpmidi(fmt, ...) if(g_debug && g_debug->rtpMidi) g_debug->debugRtpMidi(fmt, ##__VA_ARGS__)

#endif // DEBUGMANAGER_H
