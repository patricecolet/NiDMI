#pragma once

#include "../ComponentDefinition.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageCatalog.h"
#include "../../utils/PinMapper.h"

/**
 * @file VelostatDef.h
 * @brief Définition du composant Velostat
 * 
 * Capteur de pression/flexion analogique.
 * Envoie Note On/Off selon un seuil et Key Pressure (Polyphonic Aftertouch) pendant l'activation.
 * 
 * Famille: BASIC
 */

namespace Components {

/**
 * @brief Configuration spécifique au velostat
 */
struct VelostatConfig {
    uint8_t filter_intensity;      // Intensité du filtrage (1-10)
    uint16_t velocityThreshold;    // Seuil pour Note On (0-4095)
    uint16_t velocityMax;          // Valeur ADC correspondant à la vélocité 127
    uint8_t aftertouchThreshold;   // Sensibilité aftertouch (1-127)
    uint8_t scanTimeMs;            // Fenêtre de détection du pic de frappe (0 = désactivé)
    uint8_t maskTimeMs;            // Anti-redéclenchement après la Note On (0 = désactivé)

    VelostatConfig() : filter_intensity(5), velocityThreshold(50), velocityMax(4095),
                       aftertouchThreshold(4), scanTimeMs(5), maskTimeMs(30) {}
};

/**
 * @brief Définition complète du Velostat
 */
struct Velostat {
    // Identifiants
    static constexpr const char* ID = "velostat";
    static constexpr const char* DISPLAY_NAME = "Velostat";
    static constexpr const char* FAMILY_NAME = "Basic";
    
    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::BASIC;
    static constexpr ComponentType TYPE = ComponentType::VELOSTAT;
    static constexpr PinType PIN_TYPE = PinType::PIN_ANALOG;
    static constexpr bool IMPLEMENTED = true;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC = true;
    
    // Valeurs par défaut
    static constexpr uint8_t DEFAULT_NOTE = 60;
    static constexpr uint8_t DEFAULT_CHANNEL = 1;
    static constexpr uint8_t DEFAULT_FILTER_INTENSITY = 5;
    static constexpr uint16_t DEFAULT_VELOCITY_THRESHOLD = 50;  // 0-4095
    static constexpr uint8_t DEFAULT_AFTERTOUCH_THRESHOLD = 4;  // 1-127
    
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
            makeNumberFieldWithHint("velocityThreshold", "Seuil vélocité", 0, 4095, "50",
                "Seuil pour Note On (0-4095)", 60),
            makeNumberFieldWithHint("velocityMax", "ADC pour vélocité 127", 1, 4095, "4095",
                "Baisser si la vélocité max est dure à atteindre", 60),
            makeNumberFieldWithHint("aftertouchThreshold", "Seuil aftertouch", 1, 127, "4",
                "Sensibilité aftertouch (1-127)", 60),
            makeNumberFieldWithHint("filterIntensity", "Intensité filtrage (1-10)", 1, 10, "5",
                "1=rapide, 10=stable", 60),
            makeInfoField("veloWiring",
                "Câblage : capteur entre 3V3 et la pin, résistance 10 kΩ entre la pin et GND. "
                "Au repos la tension doit être proche de 0 V et monter à la pression. "
                "Une résistance trop élevée (≥100 kΩ) fait détecter la pin comme non câblée "
                "et rend le composant muet."),
            makeNumberFieldWithHint("scanTimeMs", "Fenêtre de frappe (ms)", 0, 20, "5",
                "0=vélocité au seuil, 5=pic de frappe", 60),
            makeNumberFieldWithHint("maskTimeMs", "Anti-rebond (ms)", 0, 200, "30",
                "Ignore les redéclenchements après la note", 60),
        };
        /* cc + range, dependsOnRole velostat */
        static constexpr MidiParamDef CC_RANGE[] = {
            {"midiCc",      "{{t.pins.cc}}:",        FieldType::NUMBER, 0, 127, "7", "7", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr},
            {"midiChannel", "{{t.pins.channel}}:",   FieldType::NUMBER, 1, 16,  "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr},
            {"midiCcRange", "{{t.pins.midiRange}}:", FieldType::RANGE,  0, 127, nullptr, nullptr, "0", "127", "→", nullptr, nullptr, 90, "[\"velostat\"]"},
        };
        static constexpr MidiMessageDef MM[] = {
            msgNoteKeyPressure(),
            {"cc", "Control Change", "CC#{cc}", nullptr, 3, CC_RANGE, 3},
            msgPitchBend(), msgAftertouch(), msgNoteSweep(),
        };
        return makeFlashDef(ID, DISPLAY_NAME, "cardVelostat", FAMILY, FAMILY_NAME, TYPE, PIN_TYPE,
                            SUPPORTS_MIDI, SUPPORTS_OSC, IMPLEMENTED, FF, 7, MM, 5);
    }
};

} // namespace Components
