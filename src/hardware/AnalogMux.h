#pragma once

#include <Arduino.h>

/**
 * @brief Multiplexeur analogique 16 canaux (HC4067, CD74HC4067, etc.)
 * 
 * Implementation basee sur Control-Surface:
 * - EN optionnel (255 = NO_PIN si non connectee)
 * - Activation/desactivation automatique a chaque lecture
 * - Discard first reading pour stabilite
 * - EN active LOW
 */
class AnalogMux {
public:
    static constexpr uint8_t NO_PIN = 255;
    static constexpr uint8_t NUM_CHANNELS = 16;
    
    /**
     * @brief Constructeur
     * @param sigPin Pin analogique (SIG/COM)
     * @param s0 Pin de selection S0
     * @param s1 Pin de selection S1
     * @param s2 Pin de selection S2
     * @param s3 Pin de selection S3
     * @param enPin Pin enable (optionnel, NO_PIN si non connectee)
     */
    AnalogMux(uint8_t sigPin, uint8_t s0, uint8_t s1, uint8_t s2, uint8_t s3, 
              uint8_t enPin = NO_PIN)
        : sig(sigPin), en(enPin), discardFirstReading_(true) {
        s[0] = s0; s[1] = s1; s[2] = s2; s[3] = s3;
    }
    
    /**
     * @brief Initialiser le multiplexeur
     */
    void begin() {
        for (uint8_t i = 0; i < 4; i++) {
            pinMode(s[i], OUTPUT);
            digitalWrite(s[i], LOW);
        }
        if (en != NO_PIN) {
            pinMode(en, OUTPUT);
            digitalWrite(en, HIGH); // Disabled by default (active LOW)
        }
    }
    
    /**
     * @brief Lire une valeur analogique sur un canal
     * @param channel Canal a lire (0-15)
     * @return Valeur analogique (0-4095)
     */
    uint16_t read(uint8_t channel) {
        if (channel >= NUM_CHANNELS) return 0;
        
        prepareReading(channel);
        if (discardFirstReading_)
            (void)analogRead(sig);  // Jeter premiere lecture (stabilite)
        uint16_t val = analogRead(sig);
        afterReading();
        return val;
    }
    
    /**
     * @brief Activer/desactiver le discard first reading
     */
    void setDiscardFirstReading(bool discard) {
        discardFirstReading_ = discard;
    }
    
    /**
     * @brief Lire tous les canaux en une seule passe (optimisé)
     * @param values Tableau de sortie pour les 16 valeurs (0-4095)
     * @return true si succès, false si erreur
     */
    bool readAll(uint16_t* values) {
        if (!values) return false;
        
        // Lire tous les canaux en séquence optimisée
        for (uint8_t ch = 0; ch < NUM_CHANNELS; ch++) {
            prepareReading(ch);
            if (discardFirstReading_)
                (void)analogRead(sig);  // Jeter première lecture
            values[ch] = analogRead(sig);
            afterReading();
        }
        return true;
    }
    
    /**
     * @brief Obtenir la pin SIG
     */
    uint8_t getSigPin() const { return sig; }
    
    /**
     * @brief Obtenir la pin EN
     */
    uint8_t getEnPin() const { return en; }

private:
    uint8_t sig;
    uint8_t s[4];
    uint8_t en;
    bool discardFirstReading_;
    
    /**
     * @brief Selectionner l'adresse du multiplexeur
     * Utilise la méthode de Control-Surface avec masque décalé pour garantir l'ordre
     */
    inline void setMuxAddress(uint8_t channel) {
        uint8_t mask = 1;
        for (uint8_t i = 0; i < 4; i++) {
            digitalWrite(s[i], (channel & mask) ? HIGH : LOW);
            mask <<= 1;
        }
        delayMicroseconds(10);  // Stabilisation (SELECT_LINE_DELAY)
    }
    
    /**
     * @brief Activer le multiplexeur (EN = LOW)
     */
    inline void enable() {
        if (en != NO_PIN) digitalWrite(en, LOW);
    }
    
    /**
     * @brief Desactiver le multiplexeur (EN = HIGH)
     */
    inline void disable() {
        if (en != NO_PIN) digitalWrite(en, HIGH);
    }
    
    /**
     * @brief Preparer la lecture (adresse + enable)
     */
    inline void prepareReading(uint8_t channel) {
        setMuxAddress(channel);
        enable();
    }
    
    /**
     * @brief Finaliser la lecture (disable)
     */
    inline void afterReading() {
        disable();
    }
};
