#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageFactory.h"
#include "../../midi/MidiMessageType.h"

/**
 * @file Mpr121Def.h
 * @brief Définition du composant MPR121 (touch capacitif 12 canaux, Grove)
 *
 * Capteur capacitif MPR121, 12 électrodes (ELE0–ELE11), interface I2C.
 * Chaque électrode peut envoyer Note On/Off (note = base + index) ou CC (control = base + index, 127/0).
 *
 * Famille: INTERFACE
 * Interface: I2C (adresses 0x5A, 0x5B, 0x5C, 0x5D)
 */
namespace Components {

/**
 * @brief Configuration spécifique au MPR121
 */
struct Mpr121Config {
    uint8_t i2c_address;       // 90=0x5A, 91=0x5B, 92=0x5C, 93=0x5D
    uint8_t base_note;          // Note de base (électrode 0) ou premier CC
    uint8_t midi_channel;       // Canal MIDI (1–16)
    MidiMessageType msg_type;   // NOTE ou CONTROL_CHANGE
    uint8_t touch_threshold;    // Seuil touch (1–50, plus bas = plus sensible)
    uint8_t release_threshold;  // Seuil release (doit être < touch_threshold)

    Mpr121Config()
        : i2c_address(90)
        , base_note(60)
        , midi_channel(1)
        , msg_type(MidiMessageType::NOTE)
        , touch_threshold(6)
        , release_threshold(3) {}
};

/**
 * @brief Définition complète du MPR121
 */
struct Mpr121 {
    // Identifiants
    static constexpr const char* ID = "mpr121";
    static constexpr const char* DISPLAY_NAME = "MPR121 (Touch capacitif 12 canaux, Grove)";
    static constexpr const char* FAMILY_NAME = "Interface";

    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::INTERFACE;
    static constexpr ComponentType TYPE = ComponentType::MPR121;
    static constexpr PinType PIN_TYPE = PinType::PIN_I2C;
    static constexpr bool IMPLEMENTED = true;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC = true;

    static constexpr uint8_t DEFAULT_CHANNEL = 1;
    static constexpr uint8_t DEFAULT_BASE_NOTE = 60;

    /**
     * @brief Validation : pour I2C, le GPIO est virtuel (pin bus)
     */
    static bool validate(uint8_t gpio) {
        (void)gpio;
        return true;
    }

    /**
     * @brief Crée la définition complète pour le registre
     */
    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardTouch")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)

            .addFormField(makeSelectField(
                "i2cAddress",
                "Adresse I2C",
                "[{\"value\":\"90\",\"label\":\"0x5A (1er module)\"},{\"value\":\"91\",\"label\":\"0x5B (2e module)\"},{\"value\":\"92\",\"label\":\"0x5C (3e module)\"},{\"value\":\"93\",\"label\":\"0x5D (4e module)\"}]",
                "90",
                "r"
            ))
            .addFormField(makeNumberField(
                "baseNote",
                "Note de base (électrode 0)",
                0, 127, "60", 1, 80, "r"
            ))
            .addFormField(makeNumberFieldWithHint(
                "touchThreshold",
                "Seuil touch (1-50)",
                1, 50, "6",
                "Plus bas = plus sensible",
                60, "r"
            ))
            .addFormField(makeNumberFieldWithHint(
                "releaseThreshold",
                "Seuil release (1-50)",
                1, 50, "3",
                "Doit être < seuil touch",
                60, "r"
            ))
            .addMidiMessage(createNoteMessage(false))
            .addMidiMessage(createCcMessage(false))
            .build();
    }
};

} // namespace Components
