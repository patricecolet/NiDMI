#pragma once

#include <Arduino.h>

// Forward declaration
class ComponentManager;

/**
 * @brief Chargeur de configuration depuis NVS
 * 
 * Charge les configurations des composants depuis la NVS
 * et les ajoute au ComponentManager.
 */
class ConfigLoader {
public:
    /**
     * @brief Charger les configurations depuis NVS
     * @param manager Référence au ComponentManager pour ajouter les composants
     */
    static void loadFromNVS(ComponentManager& manager);
};
