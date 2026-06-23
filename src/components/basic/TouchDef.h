#pragma once

#include "../ComponentDefinition.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageCatalog.h"
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
        static constexpr FormFieldDef FF[] = {
            makeTextField("s", "Seuils ON,OFF (raw)", "0,0", "0,0", 16, 80,
                "Depuis baseline: déclenchement,relâchement (0,0=auto)"),
            makeNumberFieldWithHint("aftertouchRange", "Plage aftertouch (raw)", 0, 500000, "20000",
                "Valeurs brutes au-dessus de la baseline pour modulation 0-127 (0=auto 20%)", 80),
            makeNumberFieldWithHint("filterIntensity", "Intensité filtrage (1-10)", 1, 10, "5",
                "1=rapide, 10=stable", 60),
        };
        /* cc + range, dependsOnRole touch */
        static constexpr MidiParamDef CC_RANGE[] = {
            {"midiCc",      "{{t.pins.cc}}:",        FieldType::NUMBER, 0, 127, "7", "7", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr},
            {"midiChannel", "{{t.pins.channel}}:",   FieldType::NUMBER, 1, 16,  "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr},
            {"midiCcRange", "{{t.pins.midiRange}}:", FieldType::RANGE,  0, 127, nullptr, nullptr, "0", "127", "→", nullptr, nullptr, 90, "[\"touch\"]"},
        };
        static constexpr MidiMessageDef MM[] = {
            {"cc", "Control Change", "CC#{cc}", nullptr, 3, CC_RANGE, 3},
            msgPitchBend(),
            msgAftertouch(),
            msgNoteKeyPressure(), // Note + Key Pressure (comme Velostat)
            msgNoteSweep(),       // Note simple avec balayage
        };
        return makeFlashDef(ID, DISPLAY_NAME, "cardTouch", FAMILY, FAMILY_NAME, TYPE, PIN_TYPE,
                            SUPPORTS_MIDI, SUPPORTS_OSC, IMPLEMENTED, FF, 3, MM, 5);
    }
};

} // namespace Components
