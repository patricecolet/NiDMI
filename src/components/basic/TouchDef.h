#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageFactory.h"
#include "../../utils/PinMapper.h"

/**
 * @file TouchDef.h
 * @brief Définition du composant Touch (ESP32-S3)
 *
 * Capteur tactile capacitif ESP32-S3 sur pin analogique.
 * Compatible uniquement avec ESP32-S3 (pas ESP32-C3).
 * 
 * La valeur tactile (0-4095) MONTE quand on touche et est mappée vers des messages MIDI
 * (CC, Pitch Bend, Aftertouch, Note + Key Pressure, Note simple).
 *
 * Famille: BASIC
 */

namespace Components {

/**
 * @brief Définition complète du Touch
 */
struct Touch {
    // Identifiants
    static constexpr const char* ID = "touch";
    static constexpr const char* DISPLAY_NAME = "Touch (ESP32-S3)";
    static constexpr const char* FAMILY_NAME = "Basic";
    
    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::BASIC;
    static constexpr ComponentType TYPE = ComponentType::TOUCH;
    static constexpr PinType PIN_TYPE = PinType::PIN_ANALOG;
    static constexpr bool IMPLEMENTED = true;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC = true;
    
    // Valeurs par défaut
    static constexpr uint8_t DEFAULT_NOTE = 60;
    static constexpr uint8_t DEFAULT_CHANNEL = 1;
    static constexpr uint8_t DEFAULT_FILTER_INTENSITY = 5;
    static constexpr const char* DEFAULT_SEUILS_RAW = "0,0";  // ON,OFF raw depuis baseline (0,0=auto)
    static constexpr uint32_t DEFAULT_AFTERTOUCH_RANGE = 20000;  // Plage raw pour aftertouch 0-127
<<<<<<< HEAD
    
=======

>>>>>>> main
    /**
     * @brief Validation : vérifie que le GPIO a une capacité Touch et que c'est un ESP32-S3
     */
    static bool validate(uint8_t gpio) {
        // Vérifier que c'est un ESP32-S3
        #if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ARDUINO_ESP32S3_DEV) || defined(ARDUINO_ESP32S3)
            return PinMapper::hasTouch(gpio);
        #else
            return false; // Touch non supporté sur ESP32-C3
        #endif
    }
    
    /**
     * @brief Crée la définition complète pour le registre
     */
    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardTouch debug")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeTextField(
                "s",
                "Seuils ON,OFF (raw)",
                "0,0", "0,0", 16, 80,
                "Depuis baseline: déclenchement,relâchement (0,0=auto)"
            ))
            .addFormField(makeNumberFieldWithHint(
                "aftertouchRange",
                "Plage aftertouch (raw)",
                0, 500000, "20000", "Valeurs brutes au-dessus de la baseline pour modulation 0-127 (0=auto 20%)", 80
            ))
            .addFormField(makeNumberFieldWithHint(
                "filterIntensity",
                "Intensité filtrage (1-10)",
                1, 10, "5", "1=rapide, 10=stable", 60
            ))
            // Messages MIDI disponibles
            .addMidiMessage(createCcMessage(true, "[\"touch\"]"))
            .addMidiMessage(createPitchBendMessage())
            .addMidiMessage(createAftertouchMessage())
            .addMidiMessage(createNoteWithKeyPressureMessage()) // Note + Key Pressure (comme Velostat)
            .addMidiMessage(createNoteSweepMessage()) // Note simple avec balayage
            .build();
    }
};

} // namespace Components
