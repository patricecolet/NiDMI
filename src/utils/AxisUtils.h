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

/**
 * Balayage de notes : une seule course physique [axisMin..axisMax] → [noteMin..noteMax].
 * (La normalisation -127..127 coupe la course en deux demi-axes ; ici toute la course utile compte.)
 */
inline uint8_t mapNoteSweepFromFullAxisTravel(
    int32_t rawFiltered,
    int32_t axisMin,
    int32_t axisMax,
    uint8_t noteMin,
    uint8_t noteMax,
    bool invert
) {
    if (axisMin > axisMax) {
        int32_t t = axisMin;
        axisMin = axisMax;
        axisMax = t;
    }
    int32_t v = rawFiltered;
    if (v < axisMin) {
        v = axisMin;
    }
    if (v > axisMax) {
        v = axisMax;
    }
    if (axisMin == axisMax) {
        int mid = ((int)noteMin + (int)noteMax) / 2;
        return static_cast<uint8_t>(constrain(mid, 0, 127));
    }
    long nLo = invert ? (long)noteMax : (long)noteMin;
    long nHi = invert ? (long)noteMin : (long)noteMax;
    long noteL = map(v, axisMin, axisMax, nLo, nHi);
    return static_cast<uint8_t>(constrain(noteL, 0L, 127L));
}

/**
 * Auto-off balayage (rtpNoteSweepAutoOffDelay en ms, UI / NVS).
 * - 0 → timer désactivé (la note reste jusqu’au prochain changement).
 * - > 0 → délai max en ms avant Note Off si aucune nouvelle note n’arrive.
 * Pas de plancher minimal : on respecte la valeur saisie par l’utilisateur.
 */
inline uint16_t effectiveNoteSweepAutoOffMs(uint16_t rtpNoteSweepAutoOffDelay) {
    return rtpNoteSweepAutoOffDelay;
}

