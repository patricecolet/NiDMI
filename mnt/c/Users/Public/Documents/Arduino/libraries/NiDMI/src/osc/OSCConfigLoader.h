#pragma once

#include <Arduino.h>
#include <functional>

// Forward declarations
class OSCManager;
class OSCQueue;

/**
 * @brief Chargeur de configuration OSC depuis NVS
 * 
 * Charge la configuration OSC depuis la NVS et initialise
 * osc_manager et osc_queue.
 */
class OSCConfigLoader {
public:
    /**
     * @brief Structure pour la configuration OSC
     */
    struct OSCConfig {
        String ip;
        int port;
        bool broadcast;
        String target; // "ap", "sta", "both"
    };
    
    /**
     * @brief Charger la configuration OSC depuis NVS
     * @return Configuration OSC chargée
     */
    static OSCConfig loadFromNVS();
    
    /**
     * @brief Initialiser osc_manager et osc_queue avec la configuration
     * @param config Configuration OSC
     * @param osc_manager Référence à OSCManager
     * @param osc_queue Référence à OSCQueue
     * @param messageCallback Callback pour les messages OSC entrants
     */
    static void initialize(
        const OSCConfig& config,
        OSCManager& osc_manager,
        OSCQueue& osc_queue,
        std::function<void(const String&, float, const String&)> messageCallback
    );
};
