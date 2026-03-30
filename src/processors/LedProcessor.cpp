#include "LedProcessor.h"
#include "ProcessorRegistry.h"
#include "../components/ComponentTypes.h"  // Définitions communes
#include "../components/basic/LedDef.h"
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
        if ((config.type == ComponentType::LED || config.type == ComponentType::BARGRAPH) && 
            config.midi_channel == channel && 
            config.midi_param == note) {
            
            // Allumer la LED (PWM si disponible et mode pwm, sinon digital)
            const char* ledMode = "onoff"; // Défaut
            if (config.specificConfig.led) {
                ledMode = config.specificConfig.led->ledMode;
            }
            if (strcmp(ledMode, "pwm") == 0 && PinMapper::hasPwm(config.gpio)) {
                // Mode PWM : utiliser la vélocité pour contrôler la luminosité (0-127 -> 0-255)
                uint8_t pwmValue = (velocity * 2); // 0-127 -> 0-254, on peut aller jusqu'à 255
                if (pwmValue > 255) pwmValue = 255;
                if (velocity == 127) pwmValue = 255; // Vélocité max = luminosité max
                analogWrite(config.gpio, pwmValue);
            } else {
                // Mode on/off : allumer indépendamment de la vélocité
                digitalWrite(config.gpio, HIGH);
            }
            // Serial.printf("[LedProcessor] LED GPIO%d ON (Note %d ch%d vel%d)\n", 
            //              config.gpio, note, channel, velocity);
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
        if ((config.type == ComponentType::LED || config.type == ComponentType::BARGRAPH) && 
            config.midi_channel == channel && 
            config.midi_param == note) {
            
            // Éteindre la LED (PWM si disponible et mode pwm, sinon digital)
            const char* ledMode = "onoff"; // Défaut
            if (config.specificConfig.led) {
                ledMode = config.specificConfig.led->ledMode;
            }
            if (strcmp(ledMode, "pwm") == 0 && PinMapper::hasPwm(config.gpio)) {
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
    // Chercher les LEDs et Bargraphs configurés pour ce CC/canal
    for (uint8_t i = 0; i < count; i++) {
        const ComponentConfig& config = configs[i];
        if ((config.type == ComponentType::LED || config.type == ComponentType::BARGRAPH) && 
            config.midi_channel == channel && 
            config.midi_param == control) {
            
            // Allumer/éteindre selon la valeur (PWM si disponible et mode pwm, sinon digital)
            const char* ledMode = "onoff"; // Défaut
            if (config.specificConfig.led) {
                ledMode = config.specificConfig.led->ledMode;
            }
            if (strcmp(ledMode, "pwm") == 0 && PinMapper::hasPwm(config.gpio)) {
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

void LedProcessor::handleOscMessage(
    const ComponentConfig* configs,
    uint8_t count,
    const String& address,
    float value,
    const String& arg_string
) {
    // Parser l'adresse OSC pour déterminer le type de message
    // Format attendu : 
    // - Format MIDI (3 int) : /note ou /ctl avec valeur + arg_string "param,channel"
    // - Format Float (1 float) : /note ou /ctl avec valeur normalisée (0-1)
    // - Format RAW (1 int) : adresse personnalisée avec valeur (0-4095)
    
    // Chercher les LEDs configurées pour cette adresse OSC
    for (uint8_t i = 0; i < count; i++) {
        const ComponentConfig& config = configs[i];
        if (config.type != ComponentType::LED && config.type != ComponentType::BARGRAPH) {
            continue;
        }
        
        // Vérifier si l'adresse OSC correspond
        String configAddress = String(config.osc_address);
        if (configAddress.length() == 0) {
            // Adresse par défaut selon le type de message
            if (config.msg_type == MidiMessageType::NOTE) {
                configAddress = "/note";
            } else if (config.msg_type == MidiMessageType::CONTROL_CHANGE) {
                configAddress = "/ctl";
            } else {
                continue; // Type non supporté
            }
        }
        
        if (address != configAddress) {
            continue; // Adresse ne correspond pas
        }
        
        // Parser selon le format OSC
        uint8_t midiValue = 0;
        uint8_t param = config.midi_param;
        uint8_t channel = config.midi_channel;
        
        // Si arg_string est fourni, c'est probablement le format MIDI (param,channel)
        if (arg_string.length() > 0) {
            // Format MIDI : parser "param,channel" depuis arg_string
            int commaPos = arg_string.indexOf(',');
            if (commaPos > 0) {
                param = arg_string.substring(0, commaPos).toInt();
                channel = arg_string.substring(commaPos + 1).toInt();
            }
            
            // Convertir la valeur (peut être 0-127 ou 0-1)
            if (value <= 1.0f && value >= 0.0f) {
                // Format normalisé (0-1)
                midiValue = (uint8_t)(value * 127.0f);
            } else {
                // Format direct (0-127)
                midiValue = (uint8_t)value;
                if (midiValue > 127) midiValue = 127;
            }
        } else {
            // Format Float simple : utiliser config.midi_param et config.midi_channel
            // La valeur peut être normalisée (0-1) ou directe (0-127)
            if (value <= 1.0f && value >= 0.0f) {
                // Format normalisé (0-1)
                midiValue = (uint8_t)(value * 127.0f);
            } else if (value >= 0 && value <= 127) {
                // Format direct (0-127)
                midiValue = (uint8_t)value;
            } else if (value >= 0 && value <= 4095) {
                // Format RAW possible : convertir 0-4095 -> 0-127
                midiValue = (uint8_t)((value / 4095.0f) * 127.0f);
            } else {
                continue; // Valeur invalide
            }
        }
        
        // Vérifier que les paramètres correspondent (si format MIDI avec arg_string)
        if (arg_string.length() > 0) {
            if (config.midi_param != param || config.midi_channel != channel) {
                continue; // Ne correspond pas à cette LED
            }
        }
        
        // Traiter selon le type de message
        if (config.msg_type == MidiMessageType::NOTE) {
            if (midiValue > 0) {
                // Pour Note, utiliser midiValue comme vélocité
                handleMidiNoteOn(&config, 1, channel, param, midiValue);
            } else {
                handleMidiNoteOff(&config, 1, channel, param, 0);
            }
        } else if (config.msg_type == MidiMessageType::CONTROL_CHANGE) {
            // Pour Control Change, utiliser midiValue comme valeur
            handleMidiControlChange(&config, 1, channel, param, midiValue);
        }
    }
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
static bool registered_led = ProcessorRegistry::registerProcessor(
    ComponentType::LED,
    processWrapper
);

// Enregistrer aussi pour BARGRAPH (réutilise le même processeur)
static bool registered_bargraph = ProcessorRegistry::registerProcessor(
    ComponentType::BARGRAPH,
    processWrapper
);
