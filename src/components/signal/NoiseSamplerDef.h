#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageFactory.h"
#include "../../utils/PinMapper.h"

/**
 * @file NoiseSamplerDef.h
 * @brief Définition du composant Noise Sampler (famille SIGNAL)
 *
 * Premier module de la famille SIGNAL : une source de bruit blanc externe
 * câblée sur une entrée ADC est échantillonnée par l'ESP32 à la cadence du
 * poller (100 Hz, Core 0). Deux modes :
 *   - "sandh"      : sample-and-hold — latch d'une nouvelle valeur aléatoire
 *                    toutes les `rateMs` ms, maintenue entre deux latches.
 *                    Comportement classique d'une source random CV / mélodie.
 *   - "continuous" : lecture lissée en continu (random CV bruité).
 * La valeur est convertie en MIDI/OSC et publiée dans le FluxRegistry
 * (routable vers d'autres composants via les scripts de mapping).
 *
 * Famille: SIGNAL
 */

namespace Components {

/**
 * @brief Configuration spécifique au Noise Sampler
 */
struct NoiseSamplerConfig {
    char     sampleMode[12];    // "sandh" | "continuous"
    uint16_t rateMs;            // Période de latch en mode sample-and-hold (ms)
    uint16_t inMin;             // Borne basse d'entrée ADC (0-4095)
    uint16_t inMax;             // Borne haute d'entrée ADC (0-4095)
    uint8_t  filter_intensity;  // Lissage en mode continu (1-10): 1=rapide, 10=stable

    NoiseSamplerConfig()
        : rateMs(250), inMin(0), inMax(4095), filter_intensity(3) {
        strncpy(sampleMode, "sandh", sizeof(sampleMode));
        sampleMode[sizeof(sampleMode) - 1] = '\0';
    }
};

/**
 * @brief Définition complète du Noise Sampler
 */
struct NoiseSampler {
    // Identifiants
    static constexpr const char* ID = "noiseSampler";
    static constexpr const char* DISPLAY_NAME = "Noise Sampler";
    static constexpr const char* FAMILY_NAME = "Signal";

    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::SIGNAL;
    static constexpr ComponentType TYPE = ComponentType::NOISE_SAMPLER;
    static constexpr PinType PIN_TYPE = PinType::PIN_ANALOG;
    static constexpr bool IMPLEMENTED = true;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC = true;

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
            .setBasicInfo(ID, DISPLAY_NAME, "cardNoiseSampler")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            .addFormField(makeSelectField(
                "sampleMode", "Mode",
                "[{\"value\":\"sandh\",\"label\":\"Sample & Hold\"},{\"value\":\"continuous\",\"label\":\"Continu\"}]",
                "sandh"))
            .addFormField(makeNumberFieldWithHint(
                "rateMs", "Période S&H (ms)", 10, 5000, "250",
                "mode Sample & Hold", 90, "r"))
            .addFormField(makeNumberField("inMin", "Entrée min", 0, 4095, "0", 1, 100, "f"))
            .addFormField(makeNumberField("inMax", "Entrée max", 0, 4095, "4095", 1, 100, "f"))
            .addFormField(makeNumberFieldWithHint(
                "filterIntensity", "Lissage (1-10)", 1, 10, "3",
                "mode Continu", 60, "r"))
            .addMidiMessage(createCcMessage(true, "[\"noiseSampler\"]"))
            .addMidiMessage(createNoteSweepMessage())
            .addMidiMessage(createNoteMessage())
            .build();
    }
};

} // namespace Components
