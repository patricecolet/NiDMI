#pragma once

#include "../ComponentDefinition.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageCatalog.h"
#include "../../utils/PinMapper.h"

/**
 * @file PotentiometerDef.h
 * @brief Définition du composant Potentiomètre
 * 
 * Composant d'entrée analogique simple.
 * Lit une valeur 0-4095 sur un ADC et la convertit en message MIDI.
 * 
 * Famille: BASIC
 */

namespace Components {

/**
 * @brief Configuration spécifique au potentiomètre
 */
struct PotentiometerConfig {
    uint8_t filter_intensity;  // Intensité du filtrage (1-10): 1=rapide, 10=stable
    uint16_t potMin;           // Seuil minimum (0-4095)
    uint16_t potMax;           // Seuil maximum (0-4095)
    
    PotentiometerConfig() : filter_intensity(5), potMin(0), potMax(4095) {}
};

/**
 * @brief Définition complète du Potentiomètre
 */
struct Potentiometer {
    // Identifiants
    static constexpr const char* ID = "potentiometer";
    static constexpr const char* DISPLAY_NAME = "Potentiomètre";
    static constexpr const char* FAMILY_NAME = "Basic";
    
    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::BASIC;
    static constexpr ComponentType TYPE = ComponentType::POTENTIOMETER;
    static constexpr PinType PIN_TYPE = PinType::PIN_ANALOG;
    static constexpr bool IMPLEMENTED = true;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC = true;
    
    // Valeurs par défaut
    static constexpr uint8_t DEFAULT_CC = 1;
    static constexpr uint8_t DEFAULT_CHANNEL = 1;
    static constexpr uint8_t DEFAULT_FILTER_INTENSITY = 5;
    
    /**
     * @brief Validation : vérifie que le GPIO a une capacité ADC
     */
    static bool validate(uint8_t gpio) {
        return PinMapper::hasAdc(gpio);
    }
    
    /**
     * @brief Crée la définition complète pour le registre
     */
    static ComponentDefinition createDefinition() {
        static constexpr FormFieldDef FF[] = {
            makeNumberField("potMin", "Seuil minimum", 0, 4095, "0", 1, 100, "f"),
            makeNumberField("potMax", "Seuil maximum", 0, 4095, "4095", 1, 100, "f"),
            makeNumberFieldWithHint("filterIntensity", "Intensité filtrage (1-10)", 1, 10, "5", "1=rapide, 10=stable", 60, "r"),
        };
        /* cc + range avec dependsOnRole spécifique au potentiomètre */
        static constexpr MidiParamDef CC_RANGE[] = {
            {"midiCc",      "{{t.pins.cc}}:",        FieldType::NUMBER, 0, 127, "7", "7", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr},
            {"midiChannel", "{{t.pins.channel}}:",   FieldType::NUMBER, 1, 16,  "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr},
            {"midiCcRange", "{{t.pins.midiRange}}:", FieldType::RANGE,  0, 127, nullptr, nullptr, "0", "127", "→", nullptr, nullptr, 90, "[\"potentiometer\"]"},
        };
        static constexpr MidiMessageDef MM[] = {
            {"cc", "Control Change", "CC#{cc}", nullptr, 3, CC_RANGE, 3},
            msgPc(), msgPitchBend(), msgAftertouch(), msgNoteSweep(),
        };
        return makeFlashDef(ID, DISPLAY_NAME, "cardPot", FAMILY, FAMILY_NAME, TYPE, PIN_TYPE,
                            SUPPORTS_MIDI, SUPPORTS_OSC, IMPLEMENTED, FF, 3, MM, 5);
    }
};

} // namespace Components
