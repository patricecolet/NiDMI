#include "OSCConfigLoader.h"
#include "OSCManager.h"
#include "OSCQueue.h"
#include <Preferences.h>

OSCConfigLoader::OSCConfig OSCConfigLoader::loadFromNVS() {
    Preferences prefs;
    prefs.begin("nidmi", true);
    
    OSCConfig config;
    config.target = prefs.getString("osc_target", "sta");
    config.port = prefs.getInt("osc_port", 8001);
    config.ip = prefs.getString("osc_ip", "255.255.255.255");
    config.broadcast = prefs.getBool("osc_broadcast", true);
    
    prefs.end();
    
    return config;
}

void OSCConfigLoader::initialize(
    const OSCConfig& config,
    OSCManager& osc_manager,
    OSCQueue& osc_queue,
    std::function<void(const String&, float, const String&)> messageCallback
) {
    // Initialiser osc_manager avec la config NVS
    osc_manager.begin(config.ip, config.port, 8001);
    osc_manager.setBroadcast(config.broadcast);
    osc_manager.setInterface(1);
    osc_manager.setEnabled(true);
    
    // Initialiser osc_queue avec la même config
    osc_queue.begin();
    osc_queue.setTarget(config.ip, config.port);
    osc_queue.setBroadcast(config.broadcast);
    osc_queue.setInterface(1);
    
    Serial.printf("[OSCConfigLoader] OSC Config: %s:%d (broadcast=%d)\n", 
                 config.ip.c_str(), config.port, config.broadcast);
    
    // Configurer le callback OSC pour les commandes de calibrage
    osc_manager.setMessageCallback(messageCallback);
}
