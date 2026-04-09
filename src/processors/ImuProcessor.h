#pragma once

#include "../components/ComponentTypes.h"
#include "../midi/MidiSender.h"
#include "../osc/OSCQueue.h"
#include "../utils/AnalogFilter.h"

/**
 * @brief Processeur pour les IMU (accéléromètres, gyroscopes)
 * 
 * Gère la lecture I2C des données IMU, le filtrage,
 * l'application des seuils (min, zeroMin, zeroMax, max) et l'envoi
 * de messages MIDI selon la configuration de chaque axe.
 */
class ImuProcessor {
public:
    /**
     * @brief Traite un composant IMU
     * 
     * @param config Configuration du composant
     * @param state État du composant
     * @param xFilter Filtre pour l'axe X
     * @param yFilter Filtre pour l'axe Y
     * @param zFilter Filtre pour l'axe Z
     * @param midi_sender Sender MIDI pour envoyer les messages
     * @param osc_queue Queue OSC pour envoyer les messages OSC
     * @param lastXNormPtr Pointeur vers la dernière valeur normalisée X (avec offset 127)
     * @param lastYNormPtr Pointeur vers la dernière valeur normalisée Y (avec offset 127)
     * @param lastZNormPtr Pointeur vers la dernière valeur normalisée Z (avec offset 127)
     */
    static void process(
        const ComponentConfig& config,
        ComponentState& state,
        AnalogFilter& xFilter,
        AnalogFilter& yFilter,
        AnalogFilter& zFilter,
        MidiSender* midi_sender,
        OSCQueue& osc_queue,
        uint8_t* lastXNormPtr = nullptr,
        uint8_t* lastYNormPtr = nullptr,
        uint8_t* lastZNormPtr = nullptr
    );
    
private:
    /**
     * @brief Mappe une valeur brute signée vers une valeur normalisée (-127..127)
     * @param value Valeur brute filtrée (int16_t)
     * @param min Valeur minimale utile
     * @param zeroMin Début de la zone morte
     * @param zeroMax Fin de la zone morte
     * @param max Valeur maximale utile
     * @param invert true pour inverser le signe de la valeur normalisée
     * @return Valeur normalisée (-127..127)
     */
    static int8_t mapAxisValue(
        int16_t value,
        int16_t min,
        int16_t zeroMin,
        int16_t zeroMax,
        int16_t max,
        bool invert
    );
    
    /**
     * @brief Envoie un message MIDI pour un axe
     * @param normalizedValue Valeur normalisée (-127..127), CC / pitch / aftertouch
     * @param rawAxisValue Valeur filtrée brute (même domaine que les seuils min/max) : pour NOTE_SWEEP,
     *        balayage linéaire sur toute la course [min..max] → [noteMin..noteMax]
     */
    static void sendMidiForAxis(
        MidiSender* midi_sender,
        const ComponentConfig& config,
        char axis,
        int8_t normalizedValue,
        int32_t rawAxisValue
    );
    
    /**
     * @brief Envoie un message OSC pour un axe
     * @param osc_queue Queue OSC
     * @param config Configuration du composant
     * @param axis Axe ('x', 'y' ou 'z')
     * @param normalizedValue Valeur normalisée (-127..127)
     * @param rawValue Valeur brute filtrée (int16_t)
     */
    static void sendOscForAxis(
        OSCQueue& osc_queue,
        const ComponentConfig& config,
        char axis,
        int8_t normalizedValue,
        int16_t rawValue
    );
    
    /**
     * @brief Filtre une valeur signée avec un AnalogFilter
     * Note: AnalogFilter fonctionne avec uint16_t, donc on convertit
     */
    static int16_t filterSignedValue(AnalogFilter& filter, int16_t value);
};
