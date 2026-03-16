#include "MidiOutputCoordinator.h"

void MidiOutputCoordinator::sendMidi(
    MidiSender* midi_sender,
    const ComponentConfig& config,
    uint8_t value,
    uint16_t raw_value
) {
    if (!midi_sender) return;
    
    MidiMessageHandler* handler = MidiMessageHandlerFactory::getHandler(config.msg_type);
    if (handler) {
        handler->send(midi_sender, config, value, raw_value);
    }
}

void MidiOutputCoordinator::sendOsc(
    OSCQueue& osc_queue,
    const ComponentConfig& config,
    uint8_t midi_value,
    uint16_t raw_value
) {
    if (!(config.flags & 0x02)) return; // OSC non activé
    
    OscMessageHandler* handler = OscMessageHandlerFactory::getHandler(config.flags);
    if (handler) {
        handler->send(osc_queue, config, midi_value, raw_value);
    }
}

void MidiOutputCoordinator::sendMidiAndOsc(
    MidiSender* midi_sender,
    OSCQueue& osc_queue,
    const ComponentConfig& config,
    uint8_t midi_value,
    uint16_t raw_value
) {
    sendMidi(midi_sender, config, midi_value, raw_value);
    sendOsc(osc_queue, config, midi_value, raw_value);
}
