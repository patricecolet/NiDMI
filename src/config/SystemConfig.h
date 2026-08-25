#pragma once

#include <Arduino.h>

class SystemConfig {
public:
    /* Lire si le touch est activé (par défaut: true si CONFIG_SOC_TOUCH_SENSOR_SUPPORTED est activé) */
    static bool isTouchEnabled();
    
    /* Définir si le touch est activé */
    static void setTouchEnabled(bool enabled);

    /* Nombre de composants traités par cycle de la tâche temps réel (round-robin).
       Borne le travail par cycle : avec N composants configurés et une tranche de S,
       chaque composant est revisité toutes les ceil(N/S) x 10 ms. Monter S réduit la
       latence de détection, au prix de plus de travail par cycle. */
    static const uint8_t DEFAULT_COMPONENTS_PER_CYCLE = 4;
    static const uint8_t MAX_COMPONENTS_PER_CYCLE_LIMIT = 64;
    static uint8_t componentsPerCycle();
    static void setComponentsPerCycle(uint8_t count);
};
