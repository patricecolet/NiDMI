#pragma once

#include <Arduino.h>
#include <stdint.h>

/**
 * @brief Configuration générique pour un axe analogique normalisé.
 *
 * Toutes les valeurs sont exprimées dans le même domaine que la mesure
 * (ex: 0‑4095 pour un ADC, ou plage signée pour un IMU).
 */
struct AxisRangeConfig {
    int32_t min;      ///< Valeur minimale de l'axe
    int32_t zeroMin;  ///< Début de la zone morte autour du centre
    int32_t zeroMax;  ///< Fin de la zone morte autour du centre
    int32_t max;      ///< Valeur maximale de l'axe
    bool invert;      ///< true pour inverser le signe de la valeur normalisée
};

/**
 * @brief Normalise une valeur d'axe vers l'intervalle [-127..127] avec zone morte.
 *
 * - Si value est dans [zeroMin..zeroMax] → 0
 * - Si value < zeroMin → [-127..0[
 * - Si value > zeroMax → ]0..127]
 * - Si invert est true, le résultat est multiplié par -1.
 */
inline int8_t mapAxisValueGeneric(int32_t value, const AxisRangeConfig& cfg) {
    // Zone morte
    if (value >= cfg.zeroMin && value <= cfg.zeroMax) {
        return 0;
    }

    long mapped = 0;

    // Côté minimum
    if (value < cfg.zeroMin) {
        if (value <= cfg.min) {
            mapped = -127;
        } else {
            mapped = ::map(value, cfg.min, cfg.zeroMin, -127, 0);
        }
        if (mapped < -127) mapped = -127;
        if (mapped > 0) mapped = 0;
    }
    // Côté maximum
    else if (value > cfg.zeroMax) {
        if (value >= cfg.max) {
            mapped = 127;
        } else {
            mapped = ::map(value, cfg.zeroMax, cfg.max, 0, 127);
        }
        if (mapped > 127) mapped = 127;
        if (mapped < 0) mapped = 0;
    }

    int8_t out = static_cast<int8_t>(mapped);
    return cfg.invert ? static_cast<int8_t>(-out) : out;
}

