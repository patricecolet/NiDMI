#pragma once

#include <Arduino.h>

/**
 * @brief Filtre analogique optimisé (selon ARCHITECTURE_MIDI.md)
 * 
 * Filtre passe-bas avec support pour filtre médian + passe-bas agressif
 * pour NOTE_SWEEP.
 */
struct AnalogFilter {
    float alpha;
    float filtered;
    bool initialized;
    
    // Pour le filtre médian (NOTE_SWEEP uniquement)
    uint16_t median_buffer[5];  // Buffer circulaire pour médian
    uint8_t median_index;      // Index actuel dans le buffer
    bool median_initialized;    // Si le buffer médian est rempli
    
    /**
     * @brief Fonction de mapping : 1-10 vers alpha 0.5-0.05 (inversé)
     * 1 → alpha = 0.5 (filtrage minimum, réponse rapide)
     * 10 → alpha = 0.05 (filtrage maximum, réponse lente)
     */
    static float mapFilterIntensity(uint8_t intensity) {
        if (intensity < 1) intensity = 1;
        if (intensity > 10) intensity = 10;
        
        float normalized = (intensity - 1) / 9.0f; // 0.0 (valeur=1) à 1.0 (valeur=10)
        
        float alpha_min = 0.01f;  // Filtrage maximum (valeur=10) — très lissé
        float alpha_max = 0.5f;   // Filtrage minimum (valeur=1) — très réactif
        
        // Courbe exponentielle pour une variation plus progressive
        // normalized=0 → alpha_max (0.5), normalized=1 → alpha_min (0.01)
        float alpha = alpha_max * powf(alpha_min / alpha_max, normalized);
        
        return alpha;
    }
    
    // Définir alpha depuis filter_intensity (1-10)
    void setAlphaFromIntensity(uint8_t intensity) {
        alpha = mapFilterIntensity(intensity);
    }
    
    uint16_t process(uint16_t raw) {
        if (!initialized) { 
            filtered = raw; 
            initialized = true;
            // Initialiser le buffer médian
            for (int i = 0; i < 5; i++) {
                median_buffer[i] = raw;
            }
            median_index = 0;
            median_initialized = true;
            return raw; 
        }
        filtered = alpha * raw + (1.0f - alpha) * filtered;
        return (uint16_t)filtered;
    }
    
    // Filtre médian + passe-bas agressif (pour NOTE_SWEEP)
    uint16_t processMedianAndLowpass(uint16_t raw) {
        if (!initialized) {
            filtered = raw;
            initialized = true;
            for (int i = 0; i < 5; i++) {
                median_buffer[i] = raw;
            }
            median_index = 0;
            median_initialized = true;
            return raw;
        }
        
        // 1. Ajouter la valeur au buffer médian
        median_buffer[median_index] = raw;
        median_index = (median_index + 1) % 5;
        
        // 2. Calculer la médiane (copier, trier, prendre le milieu)
        uint16_t sorted[5];
        for (int i = 0; i < 5; i++) {
            sorted[i] = median_buffer[i];
        }
        // Tri à bulles simple
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4 - i; j++) {
                if (sorted[j] > sorted[j + 1]) {
                    uint16_t temp = sorted[j];
                    sorted[j] = sorted[j + 1];
                    sorted[j + 1] = temp;
                }
            }
        }
        uint16_t median_value = sorted[2]; // Médiane de 5 valeurs
        
        // 3. Passe-bas agressif sur la médiane (alpha très petit)
        float aggressive_alpha = 0.05f; // Plus agressif que le filtre normal
        filtered = aggressive_alpha * median_value + (1.0f - aggressive_alpha) * filtered;
        
        return (uint16_t)filtered;
    }
    
    // Note: adaptFilter() supprimé - on utilise maintenant filter_intensity configuré par l'utilisateur
};
