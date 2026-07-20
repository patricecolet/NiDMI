#pragma once

#include <Arduino.h>
#include "../midi/MidiMessageType.h"
#include "../components/ComponentTypes.h"  // Définitions communes

/**
 * @brief Initialiseur de composants
 * 
 * Initialise ComponentConfig et ComponentState avec les valeurs par défaut
 * et configure le GPIO selon le type de composant.
 */
class ComponentInitializer {
public:
    /**
     * @brief Initialiser une configuration de composant avec les valeurs par défaut
     * @param config Configuration à initialiser
     * @param gpio Pin GPIO
     * @param type Type de composant
     * @param midi_param Paramètre MIDI (CC/Note/Program)
     * @param channel Canal MIDI (1-16)
     * @param msg_type Type de message MIDI
     */
    static void initializeConfig(
        ComponentConfig& config,
        uint8_t gpio,
        ComponentType type,
        uint8_t midi_param,
        uint8_t channel,
        MidiMessageType msg_type
    );
    
    /**
     * @brief Initialiser un état de composant avec les valeurs par défaut
     * @param state État à initialiser
     */
    static void initializeState(ComponentState& state);
    
    /**
     * @brief Configurer le GPIO selon le type de composant
     * @param gpio Pin GPIO
     * @param type Type de composant
     * @param config Configuration du composant (optionnel, nécessaire pour boutons avec btnPullMode)
     */
    static void setupGpio(uint8_t gpio, ComponentType type, ComponentConfig* config = nullptr);
};
