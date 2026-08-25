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
    /* Le réglage était écrit en NVS par /api/osc et relu par personne : les
       liens étaient figés en dur sur STA, quoi qu'on ait coché. À défaut de
       clé, on retombe sur l'ancienne « destination » qui portait la même
       information (« ap » ou « sta »). */
    config.links = osc_links::parseMask(prefs.getString("osc_interface", config.target));
    if (config.links == osc_links::NONE) {
        /* Ancienne config en mode « IP spécifique » (osc_target = "ip"), ou
           tout décoché : ne jamais rester sans aucun lien si la diffusion est
           active, sinon l'OSC est muet sans que rien ne le dise. */
        config.links = osc_links::AP;
    }
    
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
    osc_manager.setInterface(config.links);
    osc_manager.setEnabled(true);
    
    // Initialiser osc_queue avec la même config
    osc_queue.begin();
    osc_queue.setTarget(config.ip, config.port);
    osc_queue.setBroadcast(config.broadcast);
    osc_queue.setInterface(config.links);
    
    Serial.printf("[OSCConfigLoader] OSC Config: %s:%d (broadcast=%d, liens=%s)\n",
                 config.ip.c_str(), config.port, config.broadcast,
                 osc_links::maskToString(config.links).c_str());
    
    // Configurer le callback OSC pour les commandes de calibrage
    osc_manager.setMessageCallback(messageCallback);
}
