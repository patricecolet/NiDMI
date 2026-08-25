#include "SystemConfig.h"
#include <Preferences.h>

#ifdef ESP32
#include <sdkconfig.h>
#endif

bool SystemConfig::isTouchEnabled() {
    Preferences preferences;
    preferences.begin("nidmi", true);  // true = mode lecture
    
    // Si la clé n'existe pas, utiliser la valeur par défaut
    // Par défaut: activé si TOUCH_AVAILABLE est compilé
    #ifdef ESP32
    #if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ARDUINO_ESP32S3_DEV) || defined(ARDUINO_ESP32S3)
        #ifdef CONFIG_SOC_TOUCH_SENSOR_SUPPORTED
            bool default_value = false;  // Touch compilé = activé par défaut
        #else
            bool default_value = false;  // Touch non compilé = désactivé par défaut
        #endif
    #else
        bool default_value = false;  // Pas ESP32-S3 = désactivé
    #endif
    #else
        bool default_value = false;  // Pas ESP32 = désactivé
    #endif
    
    bool key_exists = preferences.isKey("touch_enabled");
    bool enabled = preferences.getBool("touch_enabled", default_value);
    preferences.end();
    
    // Log pour debug (une seule fois au démarrage) - RENDU TRÈS VISIBLE
    static bool logged_once = false;
    if (!logged_once) {
        Serial.println("========================================");
        Serial.println("[SystemConfig] ═══ CONFIGURATION TOUCH ═══");
        Serial.printf("[SystemConfig] Touch enabled: %s\n", enabled ? "YES ✓✓✓" : "NO ✗✗✗");
        Serial.printf("[SystemConfig] Default value: %s\n", default_value ? "YES" : "NO");
        Serial.printf("[SystemConfig] NVS key 'touch_enabled' exists: %s\n", key_exists ? "YES" : "NO");
        Serial.println("========================================");
        logged_once = true;
    }
    
    return enabled;
}

void SystemConfig::setTouchEnabled(bool enabled) {
    Preferences preferences;
    preferences.begin("nidmi", false);  // false = mode écriture
    preferences.putBool("touch_enabled", enabled);
    preferences.end();
    
    Serial.printf("[SystemConfig] Touch %s\n", enabled ? "activé" : "désactivé");
}

/* Cache : la tâche temps réel lit cette valeur à chaque cycle (100 Hz), on ne peut pas
   ouvrir la NVS aussi souvent. 0 = pas encore lu. Le setter réécrit le cache, donc un
   changement depuis l'UI s'applique sans redémarrage. */
static uint8_t s_componentsPerCycle = 0;

uint8_t SystemConfig::componentsPerCycle() {
    if (s_componentsPerCycle == 0) {
        Preferences preferences;
        preferences.begin("nidmi", true);
        uint8_t value = preferences.getUChar("rt_slice", DEFAULT_COMPONENTS_PER_CYCLE);
        preferences.end();
        if (value < 1) value = 1;
        if (value > MAX_COMPONENTS_PER_CYCLE_LIMIT) value = MAX_COMPONENTS_PER_CYCLE_LIMIT;
        s_componentsPerCycle = value;
        Serial.printf("[SystemConfig] Composants par cycle: %u (defaut %u)\n",
                      (unsigned)value, (unsigned)DEFAULT_COMPONENTS_PER_CYCLE);
    }
    return s_componentsPerCycle;
}

void SystemConfig::setComponentsPerCycle(uint8_t count) {
    if (count < 1) count = 1;
    if (count > MAX_COMPONENTS_PER_CYCLE_LIMIT) count = MAX_COMPONENTS_PER_CYCLE_LIMIT;

    Preferences preferences;
    preferences.begin("nidmi", false);
    preferences.putUChar("rt_slice", count);
    preferences.end();

    s_componentsPerCycle = count;
    Serial.printf("[SystemConfig] Composants par cycle -> %u\n", (unsigned)count);
}
