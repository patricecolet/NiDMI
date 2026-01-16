#include "LedProcessor.h"
#include "ProcessorRegistry.h"
#include "../components/ComponentTypes.h"  // Définitions communes
#include "../utils/AnalogFilter.h"
#include "../utils/PinMapper.h"

void LedProcessor::handleMidiNoteOn(
    const ComponentConfig* configs,
    uint8_t count,
    uint8_t channel,
    uint8_t note,
    uint8_t velocity
) {
    // Chercher les LEDs configurées pour cette note/canal
    for (uint8_t i = 0; i < count; i++) {
        const ComponentConfig& config = configs[i];
        if (config.type == ComponentType::LED && 
            config.midi_channel == channel && 
            config.midi_param == note) {
            
            // Allumer la LED (PWM si disponible et mode pwm, sinon digital)
            if (strcmp(config.ledMode, "pwm") == 0 && PinMapper::hasPwm(config.gpio)) {
                analogWrite(config.gpio, 255); // PWM à 100%
            } else {
                digitalWrite(config.gpio, HIGH);
            }
            // Serial.printf("[LedProcessor] LED GPIO%d ON (Note %d ch%d)\n", 
            //              config.gpio, note, channel);
        }
    }
}

void LedProcessor::handleMidiNoteOff(
    const ComponentConfig* configs,
    uint8_t count,
    uint8_t channel,
    uint8_t note,
    uint8_t velocity
) {
    // Chercher les LEDs configurées pour cette note/canal
    for (uint8_t i = 0; i < count; i++) {
        const ComponentConfig& config = configs[i];
        if (config.type == ComponentType::LED && 
            config.midi_channel == channel && 
            config.midi_param == note) {
            
            // Éteindre la LED (PWM si disponible et mode pwm, sinon digital)
            if (strcmp(config.ledMode, "pwm") == 0 && PinMapper::hasPwm(config.gpio)) {
                analogWrite(config.gpio, 0); // PWM à 0%
            } else {
                digitalWrite(config.gpio, LOW);
            }
            // Serial.printf("[LedProcessor] LED GPIO%d OFF (Note %d ch%d)\n", 
            //              config.gpio, note, channel);
        }
    }
}

void LedProcessor::handleMidiControlChange(
    const ComponentConfig* configs,
    uint8_t count,
    uint8_t channel,
    uint8_t control,
    uint8_t value
) {
    // Chercher les LEDs configurées pour ce CC/canal
    for (uint8_t i = 0; i < count; i++) {
        const ComponentConfig& config = configs[i];
        if (config.type == ComponentType::LED && 
            config.midi_channel == channel && 
            config.midi_param == control) {
            
            // Allumer/éteindre selon la valeur (PWM si disponible et mode pwm, sinon digital)
            if (strcmp(config.ledMode, "pwm") == 0 && PinMapper::hasPwm(config.gpio)) {
                // Mode PWM : utiliser la valeur directement (0-127 -> 0-255)
                uint8_t pwmValue = (value * 2); // 0-127 -> 0-254, on peut aller jusqu'à 255
                if (pwmValue > 255) pwmValue = 255;
                analogWrite(config.gpio, pwmValue);
            } else {
                // Mode on/off : seuil à 50%
                bool ledState = (value > 63);
                digitalWrite(config.gpio, ledState ? HIGH : LOW);
            }
            // Serial.printf("[LedProcessor] LED GPIO%d %s (CC %d ch%d val%d)\n", 
            //              config.gpio, ledState ? "ON" : "OFF", control, channel, value);
        }
    }
}

void LedProcessor::process(
    const ComponentConfig& config,
    ComponentState& state,
    AnalogFilter* filter,
    MidiSender* midi_sender,
    OSCQueue& osc_queue
) {
    // Les LEDs sont pilotées par MIDI entrant
    // Cette fonction est appelée dans update() mais ne fait rien
    // Le pilotage se fait via handleMidiNoteOn/Off/ControlChange
    (void)config;
    (void)state;
    (void)filter;
    (void)midi_sender;
    (void)osc_queue;
}

// Wrapper pour normaliser la signature
static void processWrapper(
    const ComponentConfig& config,
    ComponentState& state,
    AnalogFilter* filter,  // Non utilisé pour les LEDs
    MidiSender* midi_sender,  // Non utilisé pour les LEDs
    OSCQueue& osc_queue  // Non utilisé pour les LEDs
) {
    LedProcessor::process(config, state, filter, midi_sender, osc_queue);
}

// Enregistrement automatique au chargement du module
static bool registered = ProcessorRegistry::registerProcessor(
    ComponentType::LED,
    processWrapper
);
