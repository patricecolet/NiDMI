#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "DebugManager.h"

/* Forward declarations */
String getDefaultConfig(String pin);
extern "C" {
    void esp32server_requestReloadPins();
}

class ConfigCache {
private:
    static constexpr uint8_t MAX_PINS = 32;
    String cache[MAX_PINS];           /* Cache config JSON par pin */
    String pinNames[MAX_PINS];        /* Noms des pins (A0, D1, etc.) */
    bool dirty[MAX_PINS];             /* Pin modifiée depuis dernière sauvegarde? */
    unsigned long lastSave;           /* Timestamp dernière sauvegarde */
    uint8_t count;                    /* Nombre de pins en cache */
    
public:
    ConfigCache();
    
    /* Stocker une config en cache (pas de NVS) */
    void setConfig(const String& pin, const String& config);
    
    /* Récupérer config depuis cache ou NVS */
    String getConfig(const String& pin);
    
    /* Supprimer une pin du cache et de la NVS */
    void removeConfig(const String& pin);
    
    /* Auto-save si nécessaire (30 secondes) */
    void autoSave();
    
    /* Forcer la sauvegarde immédiate */
    void forceSave();
    
    /* Mettre en cache sans marquer dirty (pour synchronisation avec NVS) */
    void setConfigClean(const String& pin, const String& config);
    
private:
    /* Sauvegarder toutes les pins dirty en NVS */
    void saveAllDirty();
    
    int findPinIndex(const String& pin);
};

/* Instance globale */
extern ConfigCache g_configCache;
