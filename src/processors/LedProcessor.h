#pragma once

#include <Arduino.h>
#include "../components/ComponentTypes.h"  // Définitions communes

// Forward declarations
struct AnalogFilter;
class MidiSender;
class OSCQueue;

/**
 * @brief Processeur pour les LEDs
 * 
 * Gère le pilotage des LEDs via messages MIDI entrants.
 * Les LEDs sont pilotées par Note On/Off et Control Change.
 */
class LedProcessor {
public:
    /**
     * @brief Traiter une LED (fonction vide, les LEDs sont pilotées par MIDI)
     * @param config Configuration du composant
     * @param state État runtime du composant (non utilisé pour les LEDs)
     * @param filter Filtre analogique (non utilisé pour les LEDs)
     * @param midi_sender Envoyeur MIDI (non utilisé pour les LEDs)
     * @param osc_queue Queue OSC (non utilisé pour les LEDs)
     */
    static void process(
        const ComponentConfig& config,
        ComponentState& state,
        AnalogFilter* filter,
        MidiSender* midi_sender,
        OSCQueue& osc_queue
    );
    
    /**
     * @brief Gérer un message MIDI Note On pour les LEDs
     * @param configs Tableau des configurations de composants
     * @param count Nombre de composants
     * @param channel Canal MIDI
     * @param note Note MIDI
     * @param velocity Vélocité (non utilisée)
     */
    static void handleMidiNoteOn(
        const ComponentConfig* configs,
        uint8_t count,
        uint8_t channel,
        uint8_t note,
        uint8_t velocity
    );
    
    /**
     * @brief Gérer un message MIDI Note Off pour les LEDs
     * @param configs Tableau des configurations de composants
     * @param count Nombre de composants
     * @param channel Canal MIDI
     * @param note Note MIDI
     * @param velocity Vélocité (non utilisée)
     */
    static void handleMidiNoteOff(
        const ComponentConfig* configs,
        uint8_t count,
        uint8_t channel,
        uint8_t note,
        uint8_t velocity
    );
    
    /**
     * @brief Gérer un message MIDI Control Change pour les LEDs
     * @param configs Tableau des configurations de composants
     * @param count Nombre de composants
     * @param channel Canal MIDI
     * @param control Numéro de contrôle
     * @param value Valeur (0-127)
     */
    static void handleMidiControlChange(
        const ComponentConfig* configs,
        uint8_t count,
        uint8_t channel,
        uint8_t control,
        uint8_t value
    );

    /**
     * @brief Moduler la luminosité depuis un Key Pressure (aftertouch polyphonique)
     *
     * N'agit qu'en mode PWM : la pression module la luminosité d'une LED déjà allumée
     * par la Note On. En mode on/off l'aftertouch est ignoré, l'allumage restant piloté
     * par les Note On/Off.
     *
     * @param configs Tableau des configurations de composants
     * @param count Nombre de composants
     * @param channel Canal MIDI
     * @param note Note concernée
     * @param pressure Pression (0-127)
     */
    static void handleMidiKeyPressure(
        const ComponentConfig* configs,
        uint8_t count,
        uint8_t channel,
        uint8_t note,
        uint8_t pressure
    );

    /**
     * @brief Gérer un message OSC entrant pour les LEDs
     * @param configs Tableau des configurations de composants
     * @param count Nombre de composants
     * @param address Adresse OSC (ex: "/note", "/ctl")
     * @param value Valeur float (peut être note/CC/valeur normalisée selon format)
     * @param arg_string Argument string optionnel (peut contenir param,channel pour format MIDI)
     */
    static void handleOscMessage(
        const ComponentConfig* configs,
        uint8_t count,
        const String& address,
        float value,
        const String& arg_string
    );
};
