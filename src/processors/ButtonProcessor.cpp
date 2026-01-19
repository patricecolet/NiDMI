#include "ButtonProcessor.h"
#include "ProcessorRegistry.h"
#include "../components/ComponentTypes.h"  // Définitions communes
#include "../midi/handlers/MidiOutputCoordinator.h"

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
    
    // Fonction helper pour envoyer Note On (utilise le coordinateur)
    auto sendNoteOn = [&]() {
        MidiOutputCoordinator::sendMidiAndOsc(midi_sender, osc_queue, config, 127);
    };
    
    // Fonction helper pour envoyer Note Off (utilise le coordinateur)
    auto sendNoteOff = [&]() {
        // Pour certains types (PROGRAM_CHANGE, CLOCK, TAP_TEMPO), pas de "off"
        if (config.msg_type == MidiMessageType::PROGRAM_CHANGE ||
            config.msg_type == MidiMessageType::CLOCK ||
            config.msg_type == MidiMessageType::TAP_TEMPO) {
            return; // Pas de "off" pour ces types
        }
        // Pour les autres types, envoyer avec value=0 (le handler gère Note Off vs CC=0)
        MidiOutputCoordinator::sendMidiAndOsc(midi_sender, osc_queue, config, 0);
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
            } else {
                // Au release (défaut): mémoriser qu'on a été pressé, on enverra au Rising
                state.pulse_pending = true;
            }
        } else if (btnMode == "toggle") {
            // Mode toggle: basculer l'état à chaque Falling edge
            if (!state.toggle_state) {
                // État OFF → ON
                sendNoteOn();
                state.toggle_state = true;
                state.last_value = 127;
            } else {
                // État ON → OFF
                sendNoteOff();
                state.toggle_state = false;
                state.last_value = 0;
            }
        } else {
            // Mode press_release (défaut): Note On au Falling
            sendNoteOn();
            state.last_value = 127;
        }
    } else if (rising) {
        // Rising edge (release détecté)
        if (btnMode == "pulse") {
            // Mode pulse: envoyer Note On + Note Off seulement si on avait été pressé
            if (state.pulse_pending) {
                sendNoteOn();
                sendNoteOff();
                state.pulse_pending = false;
            }
        } else if (btnMode == "press_release") {
            // Mode press_release: Note Off au Rising
            sendNoteOff();
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
