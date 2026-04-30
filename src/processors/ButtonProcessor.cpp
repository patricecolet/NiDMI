#include "ButtonProcessor.h"
#include "ProcessorRegistry.h"
#include "../components/ComponentTypes.h"  // Définitions communes
#include "../components/basic/ButtonDef.h"
#include "../midi/handlers/MidiOutputCoordinator.h"
#include "../mapping/MappingEngine.h"

void ButtonProcessor::process(
    const ComponentConfig& config,
    ComponentState& state,
    MidiSender* midi_sender,
    OSCQueue& osc_queue
) {
    // Lecture digitale avec anti-rebond
    // Déterminer le mode pull configuré
    String pullMode = "pullup"; // Défaut
    if (config.specificConfig.button) {
        pullMode = String(config.specificConfig.button->btnPullMode);
        if (pullMode.length() == 0) {
            pullMode = "pullup"; // Défaut
        }
    }
    
    bool raw_state = digitalRead(config.gpio);
    bool pressed;
    
    if (pullMode == "pullup") {
        pressed = !raw_state; // LOW = pressed avec pullup
    } else if (pullMode == "pulldown") {
        pressed = raw_state;  // HIGH = pressed avec pulldown
    } else {
        // "none" ou autre : suppose pull externe vers VCC (comportement pullup)
        pressed = !raw_state;
    }
    
    uint32_t now = millis();

    // Prime debounce state on first run to avoid a synthetic edge at boot.
    if (state.last_time == 0) {
        state.last_button_state = pressed;
        state.prev_stable_state = pressed;
        state.last_change_time = now;
        state.last_value = pressed ? 127 : 0;
        // SCRIPT arming state (reuse note_on_time):
        // 0 = not armed yet, >0 = armed timestamp.
        state.note_on_time = 0;
        state.last_time = now;
        if (config.name && config.name[0] != '\0') {
            FluxRegistry::update(config.name, pressed ? 1.0f : 0.0f);
            Serial.printf("[ButtonProcessor] GPIO%d INIT: name='%s' state=%d\n", config.gpio, config.name, pressed ? 1 : 0);
        }
        
        // Execute setup statements (f(), s(), r() without output functions) once at init
        if (config.midiMode == MidiMode::SCRIPT && config.mappingScript[0] != '\0') {
            String script = String(config.mappingScript);
            String setupStatements = "";
            
            // Extract all setup statements (no output functions)
            int statementStart = 0;
            int statementEnd = script.indexOf(';');
            
            while (statementStart < (int)script.length()) {
                int actualEnd = (statementEnd == -1) ? script.length() : statementEnd;
                String statement = script.substring(statementStart, actualEnd);
                statement.trim();
                
                if (statement.length() > 0) {
                    // Check if this is a setup statement (no output functions)
                    bool isSetupStatement = (strstr(statement.c_str(), "note.on(") == nullptr &&
                                           strstr(statement.c_str(), "note.off(") == nullptr &&
                                           strstr(statement.c_str(), "note.out(") == nullptr &&
                                           strstr(statement.c_str(), "seq.out(") == nullptr &&
                                           strstr(statement.c_str(), "ctl.out(") == nullptr &&
                                           strstr(statement.c_str(), "osc.out(") == nullptr);
                    
                    if (isSetupStatement) {
                        if (setupStatements.length() > 0) setupStatements += ";";
                        setupStatements += statement;
                    }
                }
                
                if (statementEnd == -1) break;
                statementStart = statementEnd + 1;
                statementEnd = script.indexOf(';', statementStart);
            }
            
            // Execute setup statements once
            if (setupStatements.length() > 0) {
                Serial.printf("[ButtonProcessor] GPIO%d: Executing setup statements at init:\n  '%s'\n", config.gpio, setupStatements.c_str());
                MappingEngine::execute(setupStatements.c_str(), 0.0f, midi_sender);
            }
        }
        
        return;
    }
    
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
    
    // ALWAYS update FluxRegistry so r() can read this button's state
    if (config.name && config.name[0] != '\0') {
        FluxRegistry::update(config.name, currentStableState ? 1.0f : 0.0f);
    }
    
    // In script mode, arm after a short stabilization window regardless of idle polarity.
    // This suppresses startup/floating transients while still working with pullup or pulldown wiring.
    if (config.midiMode == MidiMode::SCRIPT && state.note_on_time == 0) {
        if ((now - state.last_time) > 300) {
            state.note_on_time = now;
            state.last_button_state = pressed;
            state.prev_stable_state = currentStableState;
            state.last_change_time = now;
            Serial.printf("[ButtonProcessor] GPIO%d script armed (stable=%d)\n", config.gpio, currentStableState ? 1 : 0);
        }
    }

    // Si pas de transition, on s'arrête là
    if (!falling && !rising) {
        return;
    }

    if (config.midiMode == MidiMode::SCRIPT && config.mappingScript[0] == '\0') {
        Serial.printf("[ButtonProcessor] WARNING: SCRIPT mode active but mappingScript empty on GPIO%d\n", config.gpio);
    }

    Serial.printf("[ButtonProcessor] GPIO%d edge=%s midiMode=%s script=%s\n",
                 config.gpio,
                 falling ? "press" : "release",
                 (config.midiMode == MidiMode::SCRIPT) ? "SCRIPT" : "RTP",
                 (config.mappingScript[0] != '\0') ? config.mappingScript : "<empty>");
    
    // Fonction helper pour envoyer Note On (utilise le coordinateur si mode RTP)
    uint32_t raw_for_event = falling ? 1 : 0; // RAW digital monitoring : 1=press, 0=release
    auto sendNoteOn = [&]() {
        uint8_t value = 127; // Défaut pour Note
        if (config.msg_type == MidiMessageType::CONTROL_CHANGE) {
            value = config.midiCcOnOffMin;
        }
        if (config.midiMode != MidiMode::SCRIPT) {
            MidiOutputCoordinator::sendMidiAndOsc(midi_sender, osc_queue, config, value);
        }
        state.last_raw_value_u32 = raw_for_event;
        state.last_midi_value_u8 = value;
        state.last_telemetry_ts = millis();
    };
    
    // Fonction helper pour envoyer Note Off (utilise le coordinateur si mode RTP)
    auto sendNoteOff = [&]() {
        if (config.msg_type == MidiMessageType::PROGRAM_CHANGE ||
            config.msg_type == MidiMessageType::CLOCK ||
            config.msg_type == MidiMessageType::TAP_TEMPO) {
            return;
        }
        uint8_t value = 0;
        if (config.msg_type == MidiMessageType::CONTROL_CHANGE) {
            value = config.midiCcOnOffMax;
        }
        if (config.midiMode != MidiMode::SCRIPT) {
            MidiOutputCoordinator::sendMidiAndOsc(midi_sender, osc_queue, config, value);
        }
        state.last_raw_value_u32 = raw_for_event;
        state.last_midi_value_u8 = value;
        state.last_telemetry_ts = millis();
    };
    
    // In script mode, button behavior must be deterministic and edge-driven,
    // independently of btnMode (pulse/toggle/press_release).
    if (config.midiMode == MidiMode::SCRIPT) {
        // If not armed yet, do not emit MIDI on this edge.
        if (state.note_on_time == 0) {
            state.last_time = now;
            return;
        }

        state.last_value = falling ? 127 : 0;
        state.last_raw_value_u32 = raw_for_event;
        state.last_midi_value_u8 = (uint8_t)state.last_value;
        state.last_telemetry_ts = now;

        if (config.mappingScript[0] != '\0') {
            const bool hasNoteOn = strstr(config.mappingScript, "note.on(") != nullptr;
            const bool hasNoteOff = strstr(config.mappingScript, "note.off(") != nullptr;
            const bool hasNoteOut = strstr(config.mappingScript, "note.out(") != nullptr;
            const bool hasSeqOut = strstr(config.mappingScript, "seq.out(") != nullptr;
            const bool hasCtlOut = strstr(config.mappingScript, "ctl.out(") != nullptr;
            const bool hasOscOut = strstr(config.mappingScript, "osc.out(") != nullptr;
            const bool hasNoteMessage = hasNoteOn || hasNoteOff || hasNoteOut;
            float scriptInput = currentStableState ? 1.0f : 0.0f;
            
            Serial.printf("[ButtonProcessor] Executing script (input=%.1f, edge=%s):\n  '%s'\n",
                         scriptInput, falling ? "PRESS" : "RELEASE", config.mappingScript);

            auto buildEdgeScript = [&](bool onPress) -> String {
                String src = String(config.mappingScript);
                String out = "";
                int start = 0;
                int end = src.indexOf(';');
                
                while (start < (int)src.length()) {
                    int actualEnd = (end == -1) ? src.length() : end;
                    String statement = src.substring(start, actualEnd);
                    statement.trim();
                    
                    if (statement.length() == 0) {
                        if (end == -1) break;
                        start = end + 1;
                        end = src.indexOf(';', start);
                        continue;
                    }
                    
                    // Check if this statement is a "setup" statement (no output functions)
                    bool isSetupStatement = (strstr(statement.c_str(), "note.on(") == nullptr &&
                                           strstr(statement.c_str(), "note.off(") == nullptr &&
                                           strstr(statement.c_str(), "note.out(") == nullptr &&
                                           strstr(statement.c_str(), "note.off(") == nullptr &&
                                           strstr(statement.c_str(), "seq.out(") == nullptr &&
                                           strstr(statement.c_str(), "ctl.out(") == nullptr &&
                                           strstr(statement.c_str(), "osc.out(") == nullptr &&
                                           strstr(statement.c_str(), "print(") == nullptr);
                    
                    // Setup statements execute on both press and release
                    if (isSetupStatement) {
                        if (out.length() > 0) out += ";";
                        out += statement;
                    } else {
                        // Output statements: filter based on edge and presence of note.off
                        bool hasNoteOnInStatement = strstr(statement.c_str(), "note.on(") != nullptr;
                        bool hasNoteOffInStatement = strstr(statement.c_str(), "note.off(") != nullptr;
                        bool hasNoteOutInStatement = strstr(statement.c_str(), "note.out(") != nullptr;
                        
                        bool includeStatement = true;
                        
                        // On press: include everything except pure note.off statements
                        if (onPress) {
                            if (hasNoteOffInStatement && !hasNoteOnInStatement && !hasNoteOutInStatement) {
                                includeStatement = false;
                            }
                        } else {
                            // On release: include only note.off, note.out, seq.out, ctl.out, osc.out
                            if (hasNoteOnInStatement && !hasNoteOffInStatement && !hasNoteOutInStatement) {
                                includeStatement = false;  // Pure note.on() - skip on release
                            }
                        }
                        
                        if (includeStatement) {
                            if (out.length() > 0) out += ";";
                            out += statement;
                        }
                    }
                    
                    if (end == -1) break;
                    start = end + 1;
                    end = src.indexOf(';', start);
                }
                return out;
            };

            // Always filter statements based on edge
            String filteredScript = buildEdgeScript(falling);
            Serial.printf("[ButtonProcessor] Filtered script for edge=%s on GPIO%d: '%s'\n",
                         falling ? "PRESS" : "RELEASE",
                         config.gpio,
                         filteredScript.c_str());
            if (filteredScript.length() == 0) {
                Serial.printf("[ButtonProcessor] WARNING: Script filtered to empty on GPIO%d, original script='%s'\n",
                             config.gpio,
                             config.mappingScript);
            }
            if (filteredScript.length() > 0) {
                MappingEngine::execute(filteredScript.c_str(), scriptInput, midi_sender);
            }
        }

        state.last_time = now;
        return;
    }

    // Déterminer le mode (défaut: press_release)
    String btnMode = "press_release"; // Défaut
    if (config.specificConfig.button) {
        btnMode = String(config.specificConfig.button->btnMode);
        if (btnMode.length() == 0) {
            btnMode = "press_release";
        }
    }
    
    // Déterminer le timing pour mode pulse (défaut: release)
    String btnPulseTiming = "release"; // Défaut
    if (config.specificConfig.button) {
        btnPulseTiming = String(config.specificConfig.button->btnPulseTiming);
        if (btnPulseTiming.length() == 0) {
            btnPulseTiming = "release";
        }
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
    
    // Update FluxRegistry only when the component has a declared name.
    if (config.name && config.name[0] != '\0') {
        FluxRegistry::update(config.name, currentStableState ? 1.0f : 0.0f);
    }
    state.last_time = now;
}
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
