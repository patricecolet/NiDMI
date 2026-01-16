#include "ButtonProcessor.h"
#include "ProcessorRegistry.h"
#include "../components/ComponentTypes.h"  // Définitions communes

void ButtonProcessor::process(
    const ComponentConfig& config,
    ComponentState& state,
    MidiSender* midi_sender,
    OSCQueue& osc_queue
) {
    // Lecture digitale avec anti-rebond
    bool pressed = !digitalRead(config.gpio); // INPUT_PULLUP: LOW = pressed
    uint32_t now = millis();
    
    // Debouncing simple et fiable
    static const unsigned long DEBOUNCE_TIME = 50; // 50ms
    
    // Détecter changement d'état
    if (pressed != state.last_button_state) {
        state.last_change_time = now;
        state.last_button_state = pressed;
    }
    
    // Attendre la fin du rebond
    if ((now - state.last_change_time) < DEBOUNCE_TIME) {
        return; // Pas encore stable
    }
    
    // État stable actuel (après debounce)
    // Avec INPUT_PULLUP : pressed = true quand bouton pressé (LOW), false quand relâché (HIGH)
    bool currentStableState = pressed;
    bool prevStableState = state.prev_stable_state;
    
    // Détecter Falling (HIGH → LOW, press) et Rising (LOW → HIGH, release)
    // Falling = transition de released (false) à pressed (true)
    // Rising = transition de pressed (true) à released (false)
    bool falling = currentStableState && !prevStableState;  // false → true = press
    bool rising = !currentStableState && prevStableState;   // true → false = release
    
    // Mettre à jour l'état stable précédent pour la prochaine itération
    state.prev_stable_state = currentStableState;
    
    // Si pas de transition, on s'arrête là
    if (!falling && !rising) {
        return;
    }
    
    // Fonction helper pour envoyer Note On
    auto sendNoteOn = [&]() {
        switch (config.msg_type) {
            case MidiMessageType::NOTE:
            case MidiMessageType::NOTE_VELOCITY:
            case MidiMessageType::NOTE_SWEEP:
                midi_sender->sendNoteOn(config.midi_channel, config.midi_param, 127);
                break;
            case MidiMessageType::CONTROL_CHANGE:
                midi_sender->sendControlChange(config.midi_channel, config.midi_param, 127);
                break;
            case MidiMessageType::PROGRAM_CHANGE:
                midi_sender->sendProgramChange(config.midi_channel, config.midi_param);
                break;
            case MidiMessageType::CLOCK:
                midi_sender->sendClock();
                break;
            case MidiMessageType::TAP_TEMPO:
                midi_sender->sendClock();
                break;
            default:
                midi_sender->sendNoteOn(config.midi_channel, config.midi_param, 127);
                break;
        }
    };
    
    // Fonction helper pour envoyer Note Off
    auto sendNoteOff = [&]() {
        switch (config.msg_type) {
            case MidiMessageType::NOTE:
            case MidiMessageType::NOTE_VELOCITY:
            case MidiMessageType::NOTE_SWEEP:
                midi_sender->sendNoteOff(config.midi_channel, config.midi_param, 0);
                break;
            case MidiMessageType::CONTROL_CHANGE:
                midi_sender->sendControlChange(config.midi_channel, config.midi_param, 0);
                break;
            case MidiMessageType::PROGRAM_CHANGE:
            case MidiMessageType::CLOCK:
            case MidiMessageType::TAP_TEMPO:
                // Pas de "off" pour ces types
                break;
            default:
                midi_sender->sendNoteOff(config.midi_channel, config.midi_param, 0);
                break;
        }
    };
    
    // Fonction helper pour envoyer OSC
    auto sendOSC = [&](uint8_t value) {
        if (config.flags & 0x02) {
            String oscAddress = (config.osc_address[0] != '\0') ? String(config.osc_address) : "/note";
            if (config.flags & 0x04) {
                osc_queue.enqueueMidi(oscAddress, config.midi_param, value, config.midi_channel);
            } else {
                osc_queue.enqueueFloat(oscAddress, value / 127.0f);
            }
        }
    };
    
    // Déterminer le mode (défaut: press_release)
    String btnMode = String(config.btnMode);
    if (btnMode.length() == 0) {
        btnMode = "press_release";
    }
    
    // Déterminer le timing pour mode pulse (défaut: release)
    String btnPulseTiming = String(config.btnPulseTiming);
    if (btnPulseTiming.length() == 0) {
        btnPulseTiming = "release";
    }
    
    // Implémenter les 3 modes
    if (falling) {
        // Falling edge (press détecté)
        if (btnMode == "pulse") {
            // Mode pulse: selon le timing configuré
            if (btnPulseTiming == "press") {
                // Au press: envoyer Note On + Note Off immédiatement
                sendNoteOn();
                sendNoteOff();
                sendOSC(127);
                sendOSC(0);
            } else {
                // Au release (défaut): mémoriser qu'on a été pressé, on enverra au Rising
                state.pulse_pending = true;
            }
        } else if (btnMode == "toggle") {
            // Mode toggle: basculer l'état à chaque Falling edge
            if (!state.toggle_state) {
                // État OFF → ON
                sendNoteOn();
                sendOSC(127);
                state.toggle_state = true;
                state.last_value = 127;
            } else {
                // État ON → OFF
                sendNoteOff();
                sendOSC(0);
                state.toggle_state = false;
                state.last_value = 0;
            }
        } else {
            // Mode press_release (défaut): Note On au Falling
            sendNoteOn();
            sendOSC(127);
            state.last_value = 127;
        }
    } else if (rising) {
        // Rising edge (release détecté)
        if (btnMode == "pulse") {
            // Mode pulse: envoyer Note On + Note Off seulement si on avait été pressé
            if (state.pulse_pending) {
                sendNoteOn();
                sendNoteOff();
                sendOSC(127);
                sendOSC(0);
                state.pulse_pending = false;
            }
        } else if (btnMode == "press_release") {
            // Mode press_release: Note Off au Rising
            sendNoteOff();
            sendOSC(0);
            state.last_value = 0;
        }
        // Pour toggle, on ne fait rien au Rising
    }
}

// Wrapper pour normaliser la signature (ajouter filter* même si non utilisé)
static void processWrapper(
    const ComponentConfig& config,
    ComponentState& state,
    AnalogFilter* filter,  // Non utilisé pour les boutons
    MidiSender* midi_sender,
    OSCQueue& osc_queue
) {
    (void)filter; // Ignorer le paramètre non utilisé
    ButtonProcessor::process(config, state, midi_sender, osc_queue);
}

// Enregistrement automatique au chargement du module
static bool registered = ProcessorRegistry::registerProcessor(
    ComponentType::BUTTON,
    processWrapper
);
