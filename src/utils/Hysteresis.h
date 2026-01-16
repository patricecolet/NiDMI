#pragma once

#include <Arduino.h>

// Hystérésis unifiée pour tous les composants - méthode Control-Surface
// Réduit directement la résolution de 12 bits (0-4095) vers 7 bits (0-127)
// BITS = nombre de bits de "zone morte" pour l'hystérésis (typiquement 2)
// Plus BITS est grand, plus la zone morte est large
template <uint8_t BITS>
struct Hysteresis {
    uint8_t prevLevel = 0;  // Stocke directement 0-127
    
    // Retourne true si la valeur a changé (après hystérésis)
    // Prend une valeur haute résolution (0-4095) et la réduit vers 0-127
    bool update(uint16_t input) {
        // Constantes pour la zone morte (comme Control-Surface)
        constexpr uint16_t margin = (1ul << BITS) - 1ul;  // Ex: BITS=2 -> margin=3
        constexpr uint16_t offset = BITS >= 1 ? (1ul << (BITS - 1)) : 0;
        constexpr uint16_t max_in = 4095;  // Valeur max d'entrée (12 bits)
        constexpr uint8_t max_out = 127;   // Valeur max de sortie (7 bits)
        
        // Remettre prevLevel (0-127) en haute résolution (0-4095) pour les calculs
        // On utilise >> 5 pour réduire, donc << 5 pour remettre en haute résolution
        uint16_t prevLevelFull = ((uint16_t)prevLevel << 5) | offset;
        
        // Calculer les bornes sur la valeur haute résolution (uint16_t)
        uint16_t lowerbound = prevLevel > 0 ? 
            (prevLevelFull > margin ? prevLevelFull - margin : 0) : 0;
        uint16_t upperbound = prevLevel < max_out ? 
            (prevLevelFull + margin > max_in ? max_in : prevLevelFull + margin) : max_in;
        
        // Comparer avec la valeur haute résolution d'entrée (comme Control-Surface)
        if (input < lowerbound || input > upperbound) {
            // Réduire la résolution seulement maintenant : 12 bits → 7 bits (>> 5)
            prevLevel = input >> 5;
            return true;  // Valeur a changé
        }
        return false;  // Pas de changement
    }
    
    uint8_t getValue() const { 
        // Retourne directement 0-127 (méthode Control-Surface)
        return prevLevel;
    }
    
    void reset(uint16_t value) {
        prevLevel = value >> 5;  // 12 bits → 7 bits
    }
};
