#pragma once

#include <Arduino.h>
#include "../ComponentDefinition.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageCatalog.h"

/**
 * @file ButtonDef.h
 * @brief Définition du composant Bouton
 * 
 * Composant d'entrée digitale simple.
 * Lit un état HIGH/LOW et envoie des messages MIDI (Note, CC, Program Change).
 * 
 * Famille: BASIC
 */

namespace Components {

/**
 * @brief Configuration spécifique au bouton
 */
struct ButtonConfig {
    char btnMode[16];         // Mode bouton: "pulse", "press_release", "toggle"
    char btnPulseTiming[16];  // Timing pour mode pulse: "press" ou "release"
    char btnPullMode[16];     // Mode pull bouton: "pullup", "pulldown", "none"
    
    ButtonConfig() {
        strncpy(btnMode, "press_release", sizeof(btnMode) - 1);
        btnMode[sizeof(btnMode) - 1] = '\0';
        strncpy(btnPulseTiming, "release", sizeof(btnPulseTiming) - 1);
        btnPulseTiming[sizeof(btnPulseTiming) - 1] = '\0';
        strncpy(btnPullMode, "pullup", sizeof(btnPullMode) - 1);
        btnPullMode[sizeof(btnPullMode) - 1] = '\0';
    }
};

/**
 * @brief Définition complète du Bouton
 */
struct Button {
    // Identifiants
    static constexpr const char* ID = "button";
    static constexpr const char* DISPLAY_NAME = "Bouton";
    static constexpr const char* FAMILY_NAME = "Basic";
    
    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::BASIC;
    static constexpr ComponentType TYPE = ComponentType::BUTTON;
    static constexpr PinType PIN_TYPE = PinType::PIN_DIGITAL;
    static constexpr bool IMPLEMENTED = true;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC = true;
    
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
     * @brief Validation : vérifie que le GPIO est valide
     */
    static bool validate(uint8_t gpio) {
        return gpio < 48;
    }
    
    /**
     * @brief Crée la définition complète pour le registre
     */
    static ComponentDefinition createDefinition() {
        static constexpr FormFieldDef FF[] = {
            makeSelectField("btnMode", "Mode bouton",
                "[{\"value\":\"pulse\",\"label\":\"Push\"},{\"value\":\"press_release\",\"label\":\"Press/Release\"},{\"value\":\"toggle\",\"label\":\"Toggle\"}]",
                "pulse"),
            makeSelectField("btnPulseTiming", "Timing Push",
                "[{\"value\":\"press\",\"label\":\"Au press\"},{\"value\":\"release\",\"label\":\"Au release\"}]",
                "press", "r", "btnMode", "[\"pulse\"]"),
            makeSelectField("btnPullMode", "Mode pull",
                "[{\"value\":\"pullup\",\"label\":\"Pull-up\"},{\"value\":\"pulldown\",\"label\":\"Pull-down\"},{\"value\":\"none\",\"label\":\"Aucun\"}]",
                "pullup"),
        };
        /* note + vélocité, dependsOnRole bouton */
        static constexpr MidiParamDef NOTE_VEL[] = {
            {"midiNote",     "{{t.pins.note}}:",     FieldType::NUMBER, 0, 127, "60",  "60",  nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr},
            {"midiChannel",  "{{t.pins.channel}}:",  FieldType::NUMBER, 1, 16,  "1",   "1",   nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr},
            {"midiVelocity", "{{t.pins.velocity}}:", FieldType::NUMBER, 1, 127, "100", "100", nullptr, nullptr, nullptr, nullptr, nullptr, 90, "[\"button\"]"},
        };
        /* cc on/off, dependsOnRole bouton */
        static constexpr MidiParamDef CC_ONOFF[] = {
            {"midiCc",      "{{t.pins.cc}}:",      FieldType::NUMBER, 0, 127, "7", "7", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr},
            {"midiChannel", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16,  "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr},
            {"midiCcOnOff", "{{t.pins.values}}:",  FieldType::RANGE,  0, 127, nullptr, nullptr, "127", "0", "→", nullptr, nullptr, 90, "[\"button\"]"},
        };
        static constexpr MidiMessageDef MM[] = {
            {"note", "Note",           "Note {note}", nullptr, 3, NOTE_VEL, 3},
            {"cc",   "Control Change", "CC#{cc}",     nullptr, 3, CC_ONOFF, 3},
            msgPc(), msgClock(),
        };
        return makeFlashDef(ID, DISPLAY_NAME, "cardBtn", FAMILY, FAMILY_NAME, TYPE, PIN_TYPE,
                            SUPPORTS_MIDI, SUPPORTS_OSC, IMPLEMENTED, FF, 3, MM, 4);
    }
};

} // namespace Components
