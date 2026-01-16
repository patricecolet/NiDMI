#include "ComponentInitializer.h"
#include "../components/ComponentTypes.h"  // Définitions communes
#include "../midi/MidiMessageType.h"

void ComponentInitializer::initializeConfig(
    ComponentConfig& config,
    uint8_t gpio,
    ComponentType type,
    uint8_t midi_param,
    uint8_t channel,
    MidiMessageType msg_type
) {
    config.gpio = gpio;
    config.type = type;
    config.midi_param = midi_param;
    config.midi_channel = channel;
    config.msg_type = msg_type;
    config.flags = 0x03; // rtp_enabled + osc_enabled par défaut
    strncpy(config.osc_address, "/ctl", sizeof(config.osc_address));
    config.osc_address[sizeof(config.osc_address)-1] = '\0';
    // Initialiser les champs pour NOTE_SWEEP
    config.rtpNoteMin = 48;  // Défaut: C3
    config.rtpNoteMax = 72;  // Défaut: C5
    config.rtpNoteVelFix = 100; // Défaut: vélocité fixe
    config.rtpNoteSweepAutoOffDelay = 0; // Défaut: désactivé
    strncpy(config.btnMode, "press_release", sizeof(config.btnMode)); // Défaut: press/release
    config.btnMode[sizeof(config.btnMode)-1] = '\0';
    strncpy(config.btnPulseTiming, "release", sizeof(config.btnPulseTiming)); // Défaut: release
    config.btnPulseTiming[sizeof(config.btnPulseTiming)-1] = '\0';
    strncpy(config.ledMode, "onoff", sizeof(config.ledMode)); // Défaut: on/off
    config.ledMode[sizeof(config.ledMode)-1] = '\0';
    config.filter_intensity = 5; // Défaut: filtre modéré (bon compromis)
}

void ComponentInitializer::initializeState(ComponentState& state) {
    state.last_value = 0;
    state.last_time = 0;
    state.debounce_state = 0;
    state.last_note = 255; // Aucune note jouée initialement
    state.note_on_time = 0; // Pas de note jouée initialement
    state.hysteresis.reset(0); // Hystérésis initialisée à 0
    state.toggle_state = false; // État toggle initialisé à false (note off)
    state.prev_stable_state = false; // État stable précédent (released par défaut)
    state.pulse_pending = false; // Pas de pulse en attente
    
    // Initialiser les champs de debouncing simple
    state.last_button_state = false;
    state.last_change_time = 0;
}

void ComponentInitializer::setupGpio(uint8_t gpio, ComponentType type) {
    switch (type) {
        case ComponentType::POTENTIOMETER:
            // ADC auto
            break;
        case ComponentType::BUTTON:
            pinMode(gpio, INPUT_PULLUP);
            break;
        case ComponentType::LED:
            pinMode(gpio, OUTPUT);
            digitalWrite(gpio, LOW);
            break;
    }
}
