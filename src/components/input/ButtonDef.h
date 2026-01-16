#pragma once

#include "../ComponentDefinition.h"

/**
 * @file ButtonDef.h
 * @brief Définition du composant Bouton
 * 
 * Composant d'entrée digitale simple.
 * Lit un état HIGH/LOW et envoie des messages MIDI (Note, CC, Program Change).
 */

namespace Components {

/**
 * @brief Constantes pour le Bouton
 */
struct Button {
    // Identifiants
    static constexpr const char* ID = "button";
    static constexpr const char* DISPLAY_NAME = "Bouton";
    
    // Configuration
    static constexpr ComponentType TYPE = ComponentType::BUTTON;
    static constexpr PinType PIN_TYPE = PinType::PIN_DIGITAL;
    static constexpr bool IMPLEMENTED = true;
    static constexpr bool IS_COMPLEX = false;
    
    // Valeurs par défaut
    static constexpr uint8_t DEFAULT_NOTE = 60;  // Middle C
    static constexpr uint8_t DEFAULT_CHANNEL = 1;
    static constexpr uint8_t DEFAULT_VELOCITY = 127;
    static constexpr uint32_t DEBOUNCE_TIME_MS = 50;
    
    // Modes de fonctionnement
    static constexpr const char* MODE_PULSE = "pulse";
    static constexpr const char* MODE_PRESS_RELEASE = "press_release";
    static constexpr const char* MODE_TOGGLE = "toggle";
    
    /**
     * @brief Validation inline pour le bouton
     * @param gpio GPIO à valider
     * @return true si le GPIO est valide (< 48 pour ESP32)
     */
    static bool validate(uint8_t gpio) {
        return gpio < 48;  // ESP32 a max 48 GPIOs
    }
    
    /**
     * @brief Crée la définition pour le registre
     */
    static ComponentDefinition createDefinition() {
        return {
            ID,
            DISPLAY_NAME,
            nullptr,
            TYPE,
            PIN_TYPE,
            IMPLEMENTED,
            IS_COMPLEX
        };
    }
};

} // namespace Components
