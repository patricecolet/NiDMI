#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageFactory.h"
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
    uint8_t aftertouchThreshold;   // Sensibilité aftertouch (1-127)
    
    VelostatConfig() : filter_intensity(5), velocityThreshold(50), aftertouchThreshold(4) {}
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
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardVelostat")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeNumberFieldWithHint(
                "velocityThreshold",
                "Seuil vélocité",
                0, 4095, "50", "Seuil pour Note On (0-4095)", 60
            ))
            .addFormField(makeNumberFieldWithHint(
                "aftertouchThreshold",
                "Seuil aftertouch",
                1, 127, "4", "Sensibilité aftertouch (1-127)", 60
            ))
            .addFormField(makeNumberFieldWithHint(
                "filterIntensity",
                "Intensité filtrage (1-10)",
                1, 10, "5", "1=rapide, 10=stable", 60
            ))
            .addMidiMessage(createNoteWithKeyPressureMessage())
            .addMidiMessage(createCcMessage(true, "[\"velostat\"]"))
            .addMidiMessage(createPitchBendMessage())
            .addMidiMessage(createAftertouchMessage())
            .addMidiMessage(createNoteSweepMessage())
            .build();
    }
};

} // namespace Components
