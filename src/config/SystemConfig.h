#pragma once

#include <Arduino.h>

class SystemConfig {
public:
    /* Lire si le touch est activé (par défaut: true si CONFIG_SOC_TOUCH_SENSOR_SUPPORTED est activé) */
    static bool isTouchEnabled();
    
    /* Définir si le touch est activé */
    static void setTouchEnabled(bool enabled);
};
