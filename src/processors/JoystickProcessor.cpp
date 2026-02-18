#include "JoystickProcessor.h"
#include "ProcessorRegistry.h"
#include "../components/ComponentTypes.h"
#include "../components/basic/JoystickDef.h"
#include "../midi/handlers/MidiOutputCoordinator.h"
#include "../midi/MidiMessageType.h"
#include "../utils/PinMapper.h"
#include "../managers/complex/joystick/JoystickHandler.h"
#include "../managers/complex/ComplexHandlerRegistry.h"

void JoystickProcessor::process(
    const ComponentConfig& config,
    ComponentState& state,
    AnalogFilter& xFilter,
    AnalogFilter& yFilter,
    uint8_t yGpio,
    MidiSender* midi_sender,
    OSCQueue& osc_queue,
    uint8_t* lastXNormPtr,
    uint8_t* lastYNormPtr
) {
    if (config.gpio >= 255 || config.gpio > 48 || yGpio >= 255 || yGpio > 48) {
        return;
    }
    
    if (!PinMapper::hasAdc(config.gpio) || !PinMapper::hasAdc(yGpio)) {
        return;
    }
    
    uint16_t xRaw = analogRead(config.gpio);
    uint16_t yRaw = analogRead(yGpio);
    
    uint8_t intensity = 5;
    uint16_t xMin = 200, xZeroMin = 1900, xZeroMax = 2100, xMax = 4000;
    uint16_t yMin = 200, yZeroMin = 1900, yZeroMax = 2100, yMax = 4000;
    
    if (config.specificConfig.joystick) {
        Components::JoystickConfig* joyConfig = config.specificConfig.joystick;
        intensity = joyConfig->filter_intensity;
        if (intensity == 0) intensity = 5;
        xMin = joyConfig->joyXMin;
        xZeroMin = joyConfig->joyXZeroMin;
        xZeroMax = joyConfig->joyXZeroMax;
        xMax = joyConfig->joyXMax;
        yMin = joyConfig->joyYMin;
        yZeroMin = joyConfig->joyYZeroMin;
        yZeroMax = joyConfig->joyYZeroMax;
        yMax = joyConfig->joyYMax;
    }
    
    xFilter.setAlphaFromIntensity(intensity);
    yFilter.setAlphaFromIntensity(intensity);
    
    uint16_t xFiltered = xFilter.process(xRaw);
    uint16_t yFiltered = yFilter.process(yRaw);
    
    int8_t xNorm = mapAxisValue(xFiltered, xMin, xZeroMin, xZeroMax, xMax);
    int8_t yNorm = mapAxisValue(yFiltered, yMin, yZeroMin, yZeroMax, yMax);
    
    int8_t lastXNorm = (lastXNormPtr && *lastXNormPtr != 255) ? (int8_t)(*lastXNormPtr - 127) : 128;
    int8_t lastYNorm = (lastYNormPtr && *lastYNormPtr != 255) ? (int8_t)(*lastYNormPtr - 127) : 128;
    
    if (xNorm != lastXNorm && lastXNormPtr) {
        sendMidiForAxis(midi_sender, config, 'x', xNorm);
        sendOscForAxis(osc_queue, config, 'x', xNorm, xFiltered);
        *lastXNormPtr = (uint8_t)(xNorm + 127);
    }
    
    if (yNorm != lastYNorm && lastYNormPtr) {
        sendMidiForAxis(midi_sender, config, 'y', yNorm);
        sendOscForAxis(osc_queue, config, 'y', yNorm, yFiltered);
        *lastYNormPtr = (uint8_t)(yNorm + 127);
    }
    
    state.last_value = xFiltered;
    state.last_time = millis();
}

int8_t JoystickProcessor::mapAxisValue(uint16_t value, uint16_t min, uint16_t zeroMin, uint16_t zeroMax, uint16_t max) {
    // Zone morte au centre
    if (value >= zeroMin && value <= zeroMax) {
        return 0;
    }
    
    // Côté minimum (gauche / bas)
    if (value < zeroMin) {
        if (value <= min) return -127;
        // Map [min, zeroMin[ -> [-127, 0[
        long mapped = map(value, min, zeroMin, -127, 0);
        if (mapped < -127) mapped = -127;
        if (mapped > 0) mapped = 0;
        return (int8_t)mapped;
    }
    
    // Côté maximum (droite / haut)
    if (value > zeroMax) {
        if (value >= max) return 127;
        // Map ]zeroMax, max] -> ]0, +127]
        long mapped = map(value, zeroMax, max, 0, 127);
        if (mapped > 127) mapped = 127;
        if (mapped < 0) mapped = 0;
        return (int8_t)mapped;
    }
    
    return 0;
}

void JoystickProcessor::sendMidiForAxis(
    MidiSender* midi_sender,
    const ComponentConfig& config,
    char axis,
    int8_t normalizedValue
) {
    if (!midi_sender) return;
    
    MidiMessageType msgType = config.msg_type;
    uint8_t channel = config.midi_channel;
    uint8_t param = config.midi_param;
    
    if (config.specificConfig.joystick) {
        Components::JoystickConfig* joyConfig = config.specificConfig.joystick;
        if (axis == 'x') {
            msgType = joyConfig->xMsgType;
            channel = joyConfig->xMidiChannel;
            param = joyConfig->xMidiParam;
        } else {
            msgType = joyConfig->yMsgType;
            channel = joyConfig->yMidiChannel;
            param = joyConfig->yMidiParam;
        }
    }
    
    // CC : centre joystick (0) → valeur MIDI 64 (pas 63)
    uint8_t midiValue;
    if (normalizedValue <= 0) {
        midiValue = (uint8_t)map(normalizedValue, -127, 0, 0, 64);
    } else {
        midiValue = (uint8_t)map(normalizedValue, 0, 127, 64, 127);
    }
    
    switch (msgType) {
        case MidiMessageType::CONTROL_CHANGE:
            midi_sender->sendControlChange(channel, param, midiValue);
            break;
        case MidiMessageType::PITCH_BEND: {
            // Pitch Bend 14-bit : centre = 8192 (0 en signé)
            int pbValue;
            if (normalizedValue <= 0) {
                pbValue = (int)((long)normalizedValue * 8192 / 127);
            } else {
                pbValue = (int)((long)normalizedValue * 8191 / 127);
            }
            midi_sender->sendPitchBend(channel, pbValue);
            break;
        }
        case MidiMessageType::AFTERTOUCH:
            midi_sender->sendAftertouch(channel, midiValue);
            break;
        case MidiMessageType::NOTE_SWEEP:
            midi_sender->sendNoteOn(channel, midiValue, 100);
            break;
        default:
            midi_sender->sendControlChange(channel, param, midiValue);
            break;
    }
}

void JoystickProcessor::sendOscForAxis(
    OSCQueue& osc_queue,
    const ComponentConfig& config,
    char axis,
    int8_t normalizedValue,
    uint16_t rawValue
) {
    if (!(config.flags & 0x02)) return;
    
    // Même centre que MIDI : 0 → 64
    uint8_t midiValue;
    if (normalizedValue <= 0) {
        midiValue = (uint8_t)map(normalizedValue, -127, 0, 0, 64);
    } else {
        midiValue = (uint8_t)map(normalizedValue, 0, 127, 64, 127);
    }
    
    char address[40];
    if (config.osc_address[0] != '\0') {
        snprintf(address, sizeof(address), "%s/%c", config.osc_address, axis);
    } else {
        snprintf(address, sizeof(address), "/joy/%c", axis);
    }
    
    if (config.flags & 0x08) {
        osc_queue.enqueueFloat(String(address), (float)rawValue);
    } else if (config.flags & 0x04) {
        osc_queue.enqueueMidi(String(address), midiValue, 0, config.midi_channel);
    } else {
        float fVal = normalizedValue / 127.0f;
        osc_queue.enqueueFloat(String(address), fVal);
    }
}

// Wrapper pour normaliser la signature avec ProcessorRegistry
// Note: Le filtre Y sera stocké dans un tableau statique indexé par GPIO X
// pour gérer plusieurs joysticks simultanément
static const uint8_t MAX_JOYSTICK_FILTERS = 16;
static AnalogFilter yFilters[MAX_JOYSTICK_FILTERS];
static uint8_t yFilterGpios[MAX_JOYSTICK_FILTERS]; // GPIO X associé à chaque filtre
static uint8_t yFilterCount = 0;

// Stocker les dernières valeurs normalisées pour chaque joystick (indexé par GPIO X)
// lastXNorm et lastYNorm stockés avec offset 127 (0-254) pour éviter les valeurs négatives
static uint8_t lastXNormValues[MAX_JOYSTICK_FILTERS];
static uint8_t lastYNormValues[MAX_JOYSTICK_FILTERS];

static uint8_t getOrCreateJoystickIndex(uint8_t xGpio) {
    // Chercher un index existant pour ce GPIO X
    for (uint8_t i = 0; i < yFilterCount; i++) {
        if (yFilterGpios[i] == xGpio) {
            return i;
        }
    }
    
    // Créer un nouvel index si possible
    if (yFilterCount < MAX_JOYSTICK_FILTERS) {
        uint8_t idx = yFilterCount++;
        yFilterGpios[idx] = xGpio;
        yFilters[idx].initialized = false; // Sera initialisé lors du premier process()
        lastXNormValues[idx] = 255; // Initialiser à valeur invalide (première exécution)
        lastYNormValues[idx] = 255; // Initialiser à valeur invalide (première exécution)
        return idx;
    }
    
    // Pas de place disponible, utiliser le premier index (pas idéal mais mieux que rien)
    return 0;
}

static void processWrapper(
    const ComponentConfig& config,
    ComponentState& state,
    AnalogFilter* xFilter,
    MidiSender* midi_sender,
    OSCQueue& osc_queue
) {
    if (xFilter == nullptr) return;
    
    ComplexHandler* handler = ComplexHandlerRegistry::getHandler("joystick");
    if (!handler) return;
    
    JoystickHandler* joystickHandler = static_cast<JoystickHandler*>(handler);
    uint8_t yGpio = joystickHandler->getYAxisGpio(config.gpio);
    
    if (yGpio == 255) return;
    
    // Obtenir ou créer un index pour ce joystick
    uint8_t idx = getOrCreateJoystickIndex(config.gpio);
    AnalogFilter* yFilter = &yFilters[idx];
    
    // Traiter le joystick avec les pointeurs vers les dernières valeurs
    JoystickProcessor::process(config, state, *xFilter, *yFilter, yGpio, midi_sender, osc_queue,
                               &lastXNormValues[idx], &lastYNormValues[idx]);
}

// Enregistrement automatique au chargement du module
static bool registered = ProcessorRegistry::registerProcessor(
    ComponentType::JOYSTICK,
    processWrapper
);
