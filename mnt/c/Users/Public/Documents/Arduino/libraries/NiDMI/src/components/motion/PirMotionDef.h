#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageFactory.h"

/**
 * @file PirMotionDef.h
 * @brief Capteur de mouvement PIR (Grove) – traité comme un bouton
 *
 * Famille: MOTION
 * Interface: sortie digitale (0/1)
 * Processeur réutilisé: ButtonProcessor (ComponentType::BUTTON)
 */

namespace Components {

struct PirMotion {
    // Identifiants
    static constexpr const char* ID = "pir_motion_grove";
    static constexpr const char* DISPLAY_NAME = "Mouvement PIR (Grove)";
    static constexpr const char* FAMILY_NAME = "Motion";

    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::MOTION;
    static constexpr ComponentType  TYPE   = ComponentType::BUTTON;
    static constexpr PinType        PIN_TYPE = PinType::PIN_DIGITAL;
    static constexpr bool IMPLEMENTED = true;  // Composant actif
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC  = true;

    static bool validate(uint8_t gpio) {
        // Même logique que Button: n'importe quel GPIO digital valide
        return gpio < 48;
    }

    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardPirMotion")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            // Modes de fonctionnement identiques au bouton (Note, CC on/off, etc.)
            .addFormField(makeSelectField(
                "btnMode",
                "Mode mouvement",
                "[{\"value\":\"pulse\",\"label\":\"Pulse (détection)\"},{\"value\":\"press_release\",\"label\":\"Press/Release\"},{\"value\":\"toggle\",\"label\":\"Toggle\"}]",
                "pulse"
            ))
            .addFormField(makeSelectField(
                "btnPulseTiming",
                "Timing Pulse",
                "[{\"value\":\"press\",\"label\":\"Au front\"},{\"value\":\"release\",\"label\":\"À la fin\"}]",
                "press",
                "r",
                "btnMode",
                "[\"pulse\"]"
            ))
            .addFormField(makeSelectField(
                "btnPullMode",
                "Mode pull",
                "[{\"value\":\"pullup\",\"label\":\"Pull-up\"},{\"value\":\"pulldown\",\"label\":\"Pull-down\"},{\"value\":\"none\",\"label\":\"Aucun\"}]",
                "pullup"
            ))
            .addMidiMessage(createNoteMessage(true, "[\"pir_motion\"]"))
            .addMidiMessage(createCcOnOffMessage("[\"pir_motion\"]"))
            .addMidiMessage(createPcMessage())
            .addMidiMessage(createClockMessage())
            .build();
    }
};

} // namespace Components

