#pragma once

#include "../components/ComponentTypes.h"
#include "../midi/MidiSender.h"
#include "../osc/OSCQueue.h"
#include "../utils/AnalogFilter.h"

/**
 * @brief Processeur pour le joystick 2 axes
 * 
 * Gère la lecture analogique des deux axes (X et Y), le filtrage,
 * l'application des seuils (min, zeroMin, zeroMax, max) et l'envoi
 * de messages MIDI selon la configuration de chaque axe.
 */
class JoystickProcessor {
public:
    /**
     * @brief Traite un composant Joystick
     * 
     * @param config Configuration du composant (contient GPIO X et configs MIDI)
     * @param state État du composant
     * @param xFilter Filtre analogique pour l'axe X
     * @param yFilter Filtre analogique pour l'axe Y
     * @param yGpio GPIO de l'axe Y (récupéré depuis JoystickHandler)
     * @param midi_sender Sender MIDI pour envoyer les messages
     * @param osc_queue Queue OSC pour envoyer les messages OSC
     * @param lastXNormPtr Pointeur vers la dernière valeur normalisée X (avec offset 127)
     * @param lastYNormPtr Pointeur vers la dernière valeur normalisée Y (avec offset 127)
     */
    static void process(
        const ComponentConfig& config,
        ComponentState& state,
        AnalogFilter& xFilter,
        AnalogFilter& yFilter,
        uint8_t yGpio,
        MidiSender* midi_sender,
        OSCQueue& osc_queue,
        uint8_t* lastXNormPtr = nullptr,
        uint8_t* lastYNormPtr = nullptr
    );

    /** Coupe les notes NOTE_SWEEP actives (état interne joystick). GPIO = pin axe X du couple. */
    static void silenceNoteSweepForGpio(uint8_t xGpio, const ComponentConfig& config, MidiSender* midi_sender);

private:
    /**
     * @brief Mappe une valeur brute vers une valeur normalisée (-127..127)
     * @param value Valeur brute filtrée (0-4095)
     * @param min Valeur minimale utile
     * @param zeroMin Début de la zone morte
     * @param zeroMax Fin de la zone morte
     * @param max Valeur maximale utile
     * @param invert true pour inverser le signe de la valeur normalisée
     * @return Valeur normalisée (-127..127)
     */
    static int8_t mapAxisValue(
        uint16_t value,
        uint16_t min,
        uint16_t zeroMin,
        uint16_t zeroMax,
        uint16_t max,
        bool invert
    );
    
    /**
     * @param rawAxisValue ADC filtré (0..4095) : pour NOTE_SWEEP, balayage sur toute la course joyXMin..joyXMax
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
     * @param axis Axe ('x' ou 'y')
     * @param normalizedValue Valeur normalisée (-127..127)
     * @param rawValue Valeur brute filtrée (0-4095)
     */
    static void sendOscForAxis(
        OSCQueue& osc_queue,
        const ComponentConfig& config,
        char axis,
        int8_t normalizedValue,
        uint16_t rawValue
    );
};
