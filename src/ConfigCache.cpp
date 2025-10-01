#include "ConfigCache.h"

/* Forward declarations */
// Preferences utilisées localement dans chaque fonction
String getDefaultConfig(String pin);
extern "C" {
    void esp32server_requestReloadPins();
}

ConfigCache::ConfigCache() : lastSave(0), count(0) {
    for (int i = 0; i < MAX_PINS; i++) {
        dirty[i] = false;
    }
}

/* Stocker une config en cache (pas de NVS) */
void ConfigCache::setConfig(const String& pin, const String& config) {
    int index = findPinIndex(pin);
    if (index == -1 && count < MAX_PINS) {
        /* Nouvelle pin */
        index = count++;
        pinNames[index] = pin;
    }
    
    if (index != -1) {
        cache[index] = config;
        dirty[index] = true;
        debug_network( "[ConfigCache] Pin %s mise en cache\n", pin.c_str());
    }
}

/* Récupérer config depuis cache ou NVS */
String ConfigCache::getConfig(const String& pin) {
    int index = findPinIndex(pin);
    if (index != -1 && !cache[index].isEmpty()) {
        return cache[index];  /* Depuis cache */
    }
    
    /* Depuis NVS */
    Preferences preferences;
    preferences.begin("esp32server", true);
    String key = "pin_" + pin;
    String config = preferences.getString(key.c_str(), "\n");
    preferences.end();
    
    if (!config.isEmpty()) {
        /* Mettre en cache pour prochaine fois */
        setConfigClean(pin, config);
        return config;
    }
    
    /* Défaut */
    return getDefaultConfig(pin);
}

/* Sauvegarder toutes les pins dirty en NVS */
void ConfigCache::saveAllDirty() {
    debug_network( "[ConfigCache] DEBUG forceSave() - count=%d\n", count);
    
    if (count == 0) {
        debug_network( "[ConfigCache] DEBUG Aucune pin en cache\n\n");
        return;
    }
    
    bool hasChanges = false;
    for (int i = 0; i < count; i++) {
        if (dirty[i]) {
            hasChanges = true;
            debug_network( "[ConfigCache] DEBUG Pin %s est dirty\n", pinNames[i].c_str());
        }
    }
    
    if (!hasChanges) {
        debug_network( "[ConfigCache] DEBUG Aucune pin dirty\n\n");
        return;
    }
    
    debug_network( "[ConfigCache] DEBUG Début sauvegarde NVS...\n\n");
    Preferences preferences;
    preferences.begin("esp32server", false);
    for (int i = 0; i < count; i++) {
        if (dirty[i]) {
            String key = "pin_" + pinNames[i];
            bool success = preferences.putString(key.c_str(), cache[i]);
            dirty[i] = false;
            debug_network( "[ConfigCache] Pin %s sauvegardée en NVS (success=%s)\n", 
                         pinNames[i].c_str(), success ? "true" : "false\n");
        }
    }
    preferences.end();
    
    lastSave = millis();
    debug_network( "[ConfigCache] Sauvegarde groupée terminée (%d pins)\n", count);
    
    /* Demander le rechargement des configs pins */
    esp32server_requestReloadPins();
}

/* Auto-save si nécessaire (30 secondes) */
void ConfigCache::autoSave() {
    if (millis() - lastSave > 30000) {  /* 30 secondes */
        saveAllDirty();
    }
}

/* Forcer la sauvegarde immédiate */
void ConfigCache::forceSave() {
    debug_network( "[ConfigCache] DEBUG forceSave() appelé\n\n");
    saveAllDirty();
}

int ConfigCache::findPinIndex(const String& pin) {
    for (int i = 0; i < count; i++) {
        if (pinNames[i] == pin) return i;
    }
    return -1;
}

/* Mettre en cache sans marquer dirty (pour lecture NVS) */
void ConfigCache::setConfigClean(const String& pin, const String& config) {
    int index = findPinIndex(pin);
    if (index == -1 && count < MAX_PINS) {
        index = count++;
        pinNames[index] = pin;
        dirty[index] = false;
    }
    
    if (index != -1) {
        cache[index] = config;
    }
}
