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
