#include "ComponentManager.h"
#include <Arduino.h> // For Serial.printf
#include <Preferences.h>
#include "ServerCore.h"
#include "OSCQueue.h"
#include "midi/MidiMessageType.h"

extern ServerCore serverCore;

ComponentManager::ComponentManager() 
    : component_count(0), midi_sender(nullptr) {
    // Initialiser les filtres
    for (int i = 0; i < MAX_COMPONENTS; i++) {
        filters[i].alpha = 0.1f;
        filters[i].initialized = false;
    }
}

ComponentManager::~ComponentManager() {
    clearAll();
}

void ComponentManager::begin(MidiSender* sender) {
    midi_sender = sender;
    loadConfigFromNVS();
    
    // Charger la configuration OSC depuis NVS
    Preferences prefs;
    prefs.begin("esp32server", true);
    String osc_target = prefs.getString("osc_target", "sta");
    int osc_port = prefs.getInt("osc_port", 8001);
    String osc_ip = prefs.getString("osc_ip", "255.255.255.255");
    bool osc_broadcast = prefs.getBool("osc_broadcast", true);
    prefs.end();

    // Initialiser osc_manager avec la config NVS
    osc_manager.begin(osc_ip, osc_port, 8001);
    osc_manager.setBroadcast(osc_broadcast);
    osc_manager.setInterface(1);
    osc_manager.setEnabled(true);
    
    // Initialiser osc_queue avec la même config
    osc_queue.begin();
    osc_queue.setTarget(osc_ip, osc_port);
    osc_queue.setBroadcast(osc_broadcast);
    osc_queue.setInterface(1);

    Serial.printf("[ComponentManager] OSC Config: %s:%d (broadcast=%d)\n", 
                 osc_ip.c_str(), osc_port, osc_broadcast);
    
    // Configuration OSC optimisée (système direct)
    
    // Serial.printf("[ComponentManager] Loaded %d components\n", component_count);
    
    printStats();
}

void ComponentManager::syncOSCConfig() {
    // Récupérer la config de osc_manager
    String target = osc_manager.getTargetIP();
    int port = osc_manager.getTargetPort();
    bool broadcast = osc_manager.isBroadcastEnabled();
    
    // Appliquer à osc_queue
    osc_queue.setTarget(target, port);
    osc_queue.setBroadcast(broadcast);
}

void ComponentManager::update() {
    if (!midi_sender) {
        static unsigned long lastLog = 0;
        if (millis() - lastLog > 10000) { // Log toutes les 10s
            Serial.println("[ComponentManager] No MIDI sender configured");
            lastLog = millis();
        }
        return;
    }
    
    // Diagnostic WiFi (toutes les 30 secondes)
    static unsigned long lastDiagnostic = 0;
    if (millis() - lastDiagnostic > 30000) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("[WiFi] Signal: %d dBm\n", WiFi.RSSI());
        }
        lastDiagnostic = millis();
    }
    
    // Log périodique du nombre de composants
    static unsigned long lastComponentLog = 0;
    // if (millis() - lastComponentLog > 30000) { // Log toutes les 30s
    //     Serial.printf("[ComponentManager] Processing %d components\n", component_count);
    //     for (uint8_t i = 0; i < component_count; i++) {
    //         const ComponentConfig& config = configs[i];
    //         const char* typeName = "Unknown";
    //         switch (config.type) {
    //             case ComponentType::POTENTIOMETER: typeName = "Potentiometer"; break;
    //             case ComponentType::BUTTON: typeName = "Button"; break;
    //             case ComponentType::LED: typeName = "LED"; break;
    //         }
    //         Serial.printf("  [%d] %s on GPIO%d, MIDI ch%d param%d\n", 
    //                      i, typeName, config.gpio, config.midi_channel, config.midi_param);
    //     }
    //     lastComponentLog = millis();
    // }

    syncOSCConfig();
    // Traiter OSC en priorité (avec queue FreeRTOS)
    osc_queue.update();
    
    for (uint8_t i = 0; i < component_count; i++) {
        // Vérifier que le composant est valide avant de le traiter
        const ComponentConfig& config = configs[i];
        if (config.gpio >= 255 || config.gpio > 48) {
            // GPIO invalide, ignorer ce composant
            continue;
        }
        
        switch (config.type) {
            case ComponentType::POTENTIOMETER:
                // Vérifier ADC avant de traiter
                if (PinMapper::hasAdc(config.gpio)) {
                    processPotentiometer(i);
                }
                break;
            case ComponentType::BUTTON:
                processButton(i);
                break;
            case ComponentType::LED:
                processLed(i);
                break;
        }
    }
}

void ComponentManager::reloadConfigs() {
    // Serial.println("[ComponentManager] Reloading configs...");
    clearAll();
    loadConfigFromNVS();
    // Serial.println("[ComponentManager] Configs reloaded");
}

void ComponentManager::processPotentiometer(uint8_t index) {
    const ComponentConfig& config = configs[index];
    ComponentState& state = states[index];
    
    // Vérifier que le GPIO est valide et a un ADC (SILENCIEUX pour éviter le spam)
    if (config.gpio >= 255 || config.gpio > 48) {
        // GPIO invalide, ne pas traiter (pas de log pour éviter le spam)
        return;
    }
    
    if (!PinMapper::hasAdc(config.gpio)) {
        // Pas d'ADC, ne pas traiter (pas de log pour éviter le spam)
        return;
    }
    
    // Lecture analogique
    uint16_t raw_value = analogRead(config.gpio);
    
    // Adaptation du filtre selon la vitesse de changement
    filters[index].adaptFilter(raw_value, state.last_value);
    
    // Filtrage : médian + passe-bas agressif pour NOTE_SWEEP, sinon filtre normal
    uint16_t filtered_value;
    if (config.msg_type == MidiMessageType::NOTE_SWEEP) {
        filtered_value = filters[index].processMedianAndLowpass(raw_value);
    } else {
        filtered_value = filters[index].process(raw_value);
    }
    
    // Conversion 0-4095 → 0-127
    uint8_t midi_value = map(filtered_value, 0, 4095, 0, 127);
    
    // ===== TRAITEMENT SPÉCIAL NOTE_SWEEP =====
    // Utilise l'hystérésis pour éviter les oscillations
    if (config.msg_type == MidiMessageType::NOTE_SWEEP) {
        
        // 1. Vérifier l'auto-off AVANT tout (éteint la note si délai écoulé)
        if (config.rtpNoteSweepAutoOffDelay > 0 && 
            state.last_note != 255 && 
            state.note_on_time > 0) {
            uint32_t elapsed = millis() - state.note_on_time;
            if (elapsed >= config.rtpNoteSweepAutoOffDelay) {
                // Délai écoulé, éteindre la note
                midi_sender->sendNoteOff(config.midi_channel, state.last_note, 0);
                state.last_note = 255;
                state.note_on_time = 0;
            }
        }
        
        // 2. Appliquer l'hystérésis sur midi_value
        // Si pas de changement significatif, on s'arrête là
        if (!state.hysteresis.update(midi_value)) {
            return; // Valeur stable, rien à faire
        }
        
        // 3. L'hystérésis a détecté un changement réel
        // Utiliser la valeur stabilisée par l'hystérésis
        uint8_t stable_midi_value = state.hysteresis.getValue();
        
        // 4. Calculer la nouvelle note
        uint8_t noteMin = config.rtpNoteMin;
        uint8_t noteMax = config.rtpNoteMax;
        uint8_t newNote;
        
        if (stable_midi_value == 0) {
            newNote = 255; // Pas de note (potentiomètre à zéro)
        } else {
            newNote = map(stable_midi_value, 1, 127, noteMin, noteMax);
        }
        
        // 5. Si la note est identique à la précédente, ne rien faire
        if (newNote == state.last_note) {
            return;
        }
        
        // 6. Éteindre l'ancienne note si elle existe
        if (state.last_note != 255) {
            midi_sender->sendNoteOff(config.midi_channel, state.last_note, 0);
        }
        
        // 7. Jouer la nouvelle note (sauf si 255)
        if (newNote != 255) {
            midi_sender->sendNoteOn(config.midi_channel, newNote, config.rtpNoteVelFix);
            state.note_on_time = (config.rtpNoteSweepAutoOffDelay > 0) ? millis() : 0;
        } else {
            state.note_on_time = 0;
        }
        
        // 8. Mettre à jour l'état
        state.last_note = newNote;
        state.last_value = stable_midi_value;
        state.last_time = millis();
        
        // 9. OSC si activé
        if (config.flags & 0x02) {
            String oscAddress = (config.osc_address[0] != '\0') ? String(config.osc_address) : "/note";
            if (config.flags & 0x04) {
                osc_queue.enqueueMidi(oscAddress, stable_midi_value, config.midi_param, config.midi_channel);
            } else {
                osc_queue.enqueueFloat(oscAddress, stable_midi_value / 127.0f);
            }
        }
        
        return; // Traitement NOTE_SWEEP terminé
    }
    
    // ===== TRAITEMENT STANDARD (autres types) =====
    // Envoyer seulement si changement significatif (seuil de 3 pour XIAO_ESP32C3)
    if (abs((int)midi_value - (int)state.last_value) >= 3) {
        // Envoyer le message MIDI selon le type configuré
        switch (config.msg_type) {
            case MidiMessageType::CONTROL_CHANGE:
                midi_sender->sendControlChange(config.midi_channel, config.midi_param, midi_value);
                break;
            case MidiMessageType::PITCH_BEND: {
                // Pitch Bend: 0-127 → -8192 à +8191 (signé, centre=0)
                int pitchBend = map(midi_value, 0, 127, -8192, 8191);
                midi_sender->sendPitchBend(config.midi_channel, pitchBend);
                break;
            }
            case MidiMessageType::AFTERTOUCH:
                midi_sender->sendAftertouch(config.midi_channel, midi_value);
                break;
            case MidiMessageType::NOTE_VELOCITY:
                // Note + vélocité: envoyer Note On avec vélocité variable
                if (midi_value > 0) {
                    midi_sender->sendNoteOn(config.midi_channel, config.midi_param, midi_value);
                } else {
                    midi_sender->sendNoteOff(config.midi_channel, config.midi_param, 0);
                }
                break;
            // NOTE_SWEEP est traité avant le switch et fait return, jamais atteint ici
            case MidiMessageType::PROGRAM_CHANGE:
                // Program Change: envoyer seulement si changement significatif
                midi_sender->sendProgramChange(config.midi_channel, midi_value);
                break;
            default:
                // Par défaut: Control Change
                midi_sender->sendControlChange(config.midi_channel, config.midi_param, midi_value);
                break;
        }
        
        // Envoyer OSC si activé (via queue prioritaire)
        if (config.flags & 0x02) { // Bit OSC enabled
            // Utiliser l'adresse OSC configurée (ou défaut si vide)
            String oscAddress = (config.osc_address[0] != '\0') ? String(config.osc_address) : "/ctl";
            
            if (config.flags & 0x04) { // Format MIDI
                osc_queue.enqueueMidi(oscAddress, midi_value, config.midi_param, config.midi_channel);
            } else { // Format float
                osc_queue.enqueueFloat(oscAddress, midi_value / 127.0f);
            }
        }
        
        // Mettre à jour last_value (NOTE_SWEEP est traité avant et fait return)
        state.last_value = midi_value;
        state.last_time = millis();
    }
}

void ComponentManager::processButton(uint8_t index) {
    const ComponentConfig& config = configs[index];
    ComponentState& state = states[index];
    
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

void ComponentManager::processLed(uint8_t index) {
    // Les LEDs sont pilotées par MIDI entrant
    // Cette fonction est appelée dans update() mais ne fait rien
    // Le pilotage se fait via handleMidiNoteOn/Off
}

bool ComponentManager::addComponent(uint8_t gpio, ComponentType type, uint8_t midi_param, uint8_t channel, MidiMessageType msg_type) {
    if (component_count >= MAX_COMPONENTS) {
        Serial.printf("[ComponentManager] ERROR: Max components reached (%d)\n", MAX_COMPONENTS);
        return false;
    }
    
    // Vérifier que le GPIO est valide (0-48 pour ESP32-C3/S3)
    if (gpio >= 255 || gpio > 48) {
        Serial.printf("[ComponentManager] ERROR: Invalid GPIO %d (must be 0-48)\n", gpio);
        return false;
    }
    
    // Vérifier si le GPIO existe déjà
    if (findComponentByGpio(gpio) != 255) {
        Serial.printf("[ComponentManager] WARNING: GPIO %d already exists, skipping\n", gpio);
        return false;
    }
    
    // Vérifier que la pin a un ADC si c'est un potentiomètre
    if (type == ComponentType::POTENTIOMETER && !PinMapper::hasAdc(gpio)) {
        Serial.printf("[ComponentManager] ERROR: GPIO %d does not have ADC for potentiometer\n", gpio);
        return false;
    }
    
    // Ajouter le composant
    ComponentConfig& config = configs[component_count];
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
    
    // Serial.printf("[ComponentManager] Added component: GPIO%d, type=%d, param=%d, channel=%d, msg_type=%d\n",
    //               gpio, (int)type, midi_param, channel, (int)msg_type);
    
    // Initialiser l'état
    ComponentState& state = states[component_count];
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
    
    // Configurer le GPIO
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
    
    component_count++;
    return true;
}

bool ComponentManager::removeComponent(uint8_t gpio) {
    uint8_t index = findComponentByGpio(gpio);
    if (index == 255) return false;
    
    // Éteindre la note si c'est un NOTE_SWEEP avec une note active
    if (configs[index].msg_type == MidiMessageType::NOTE_SWEEP && states[index].last_note != 255) {
        if (midi_sender) {
            midi_sender->sendNoteOff(configs[index].midi_channel, states[index].last_note, 0);
        }
    }
    
    // Déplacer les éléments suivants
    for (uint8_t i = index; i < component_count - 1; i++) {
        configs[i] = configs[i + 1];
        states[i] = states[i + 1];
        filters[i] = filters[i + 1];
    }
    
    component_count--;
    return true;
}

void ComponentManager::clearAll() {
    // Éteindre toutes les notes actives avant de tout effacer
    for (uint8_t i = 0; i < component_count; i++) {
        if (configs[i].msg_type == MidiMessageType::NOTE_SWEEP && states[i].last_note != 255) {
            if (midi_sender) {
                midi_sender->sendNoteOff(configs[i].midi_channel, states[i].last_note, 0);
            }
        }
    }
    component_count = 0;
    // Réinitialiser les filtres
    for (uint8_t i = 0; i < MAX_COMPONENTS; i++) {
        filters[i].initialized = false;
    }
}

uint8_t ComponentManager::findComponentByGpio(uint8_t gpio) const {
    for (uint8_t i = 0; i < component_count; i++) {
        if (configs[i].gpio == gpio) return i;
    }
    return 255; // Non trouvé
}

void ComponentManager::loadConfigFromNVS() {
    Preferences preferences;
    preferences.begin("esp32server", true);
    
    // Serial.println("[ComponentManager] Loading configs from NVS...");
    // Serial.printf("[ComponentManager] Component count before: %d\n", component_count);
    
    // Charger les configurations depuis NVS
    // Les clés sont sauvegardées comme "pin_A0", "pin_D2", etc.
    String pinLabels[] = {"A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8", "A9", "A10", "A11", "A12", "A13", "A14", "A15", "A16", "A17", "A18", "A19", "A20", "A21", "A22", "A23", "A24", "A25", "A26", "A27", "A28", "A29", "A30", "A31", "A32", "A33", "A34", "A35", "A36", "A37", "A38", "A39", "A40", "A41", "A42", "A43", "A44", "A45", "A46", "A47", "A48", "A49", "A50", "A51", "A52", "A53", "A54", "A55", "A56", "A57", "A58", "A59", "A60", "A61", "A62", "A63", "A64", "A65", "A66", "A67", "A68", "A69", "A70", "A71", "A72", "A73", "A74", "A75", "A76", "A77", "A78", "A79", "A80", "A81", "A82", "A83", "A84", "A85", "A86", "A87", "A88", "A89", "A90", "A91", "A92", "A93", "A94", "A95", "A96", "A97", "A98", "A99", "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "D10", "D11", "D12", "D13", "D14", "D15", "D16", "D17", "D18", "D19", "D20", "D21", "D22", "D23", "D24", "D25", "D26", "D27", "D28", "D29", "D30", "D31", "D32", "D33", "D34", "D35", "D36", "D37", "D38", "D39", "D40", "D41", "D42", "D43", "D44", "D45", "D46", "D47", "D48", "D49", "D50", "D51", "D52", "D53", "D54", "D55", "D56", "D57", "D58", "D59", "D60", "D61", "D62", "D63", "D64", "D65", "D66", "D67", "D68", "D69", "D70", "D71", "D72", "D73", "D74", "D75", "D76", "D77", "D78", "D79", "D80", "D81", "D82", "D83", "D84", "D85", "D86", "D87", "D88", "D89", "D90", "D91", "D92", "D93", "D94", "D95", "D96", "D97", "D98", "D99"};
    
    for (int i = 0; i < 200; i++) { // Max 200 pins possibles
        String pinLabel = pinLabels[i];
        String key = "pin_" + pinLabel;
        
        if (!preferences.isKey(key.c_str())) {
            continue; // Passer au suivant
        }
        
        String pinConfig = preferences.getString(key.c_str(), "");
        if (pinConfig.length() == 0) {
            // Serial.printf("[ComponentManager] Empty config for pin: %s\n", pinLabel.c_str());
            continue;
        }
        
        // Serial.printf("[ComponentManager] Found pin: %s -> %s\n", pinLabel.c_str(), pinConfig.c_str());
        
        // Parser JSON simple
        String role = extractStr(pinConfig, "role", "\n");
        if (role.length() == 0) continue;
        
        // Utiliser PinMapper pour obtenir le GPIO
        uint8_t gpio = PinMapper::labelToGpio(pinLabel);
        if (gpio == 255) {
            Serial.printf("[ComponentManager] Invalid pin label: %s (GPIO=255)\n", pinLabel.c_str());
            continue;
        }
        
        // Vérifier que la pin a un ADC si c'est un potentiomètre
        if (role == "Potentiomètre") {
            if (!PinMapper::hasAdc(gpio)) {
                Serial.printf("[ComponentManager] WARNING: Pin %s (GPIO%d) n'a pas d'ADC, ignorée\n", 
                              pinLabel.c_str(), gpio);
                continue;
            }
        }
        
        // Log pour debug AVANT d'ajouter (seulement si GPIO valide)
        if (gpio < 255 && gpio <= 48) {
            // Serial.printf("[ComponentManager] Loading pin: %s -> GPIO%d, role: %s\n", 
            //               pinLabel.c_str(), gpio, role.c_str());
        }
        
        // Extraire paramètres MIDI
        uint8_t midi_param = 7; // défaut CC
        uint8_t channel = 1;    // défaut canal 1
        MidiMessageType msg_type = MidiMessageType::NOTE; // défaut
        
        // Lire rtpType depuis la config
        String rtpTypeStr = extractStr(pinConfig, "rtpType", "");
        if (rtpTypeStr.length() > 0) {
            msg_type = stringToMidiMessageType(rtpTypeStr);
        } else {
            // Défaut selon le rôle si rtpType n'est pas spécifié
            if (role == "Potentiomètre") {
                msg_type = MidiMessageType::CONTROL_CHANGE;
            } else if (role == "Bouton") {
                msg_type = MidiMessageType::NOTE;
            }
        }
        
        // Extraire le paramètre MIDI selon le type de message
        if (role == "Potentiomètre") {
            if (msg_type == MidiMessageType::CONTROL_CHANGE) {
                midi_param = extractInt(pinConfig, "rtpCc", 7);
            } else if (msg_type == MidiMessageType::PROGRAM_CHANGE) {
                midi_param = extractInt(pinConfig, "rtpPc", 0);
            } else if (msg_type == MidiMessageType::NOTE || msg_type == MidiMessageType::NOTE_VELOCITY || msg_type == MidiMessageType::NOTE_SWEEP) {
                midi_param = extractInt(pinConfig, "rtpNote", 60);
            }
        } else if (role == "Bouton") {
            if (msg_type == MidiMessageType::NOTE || msg_type == MidiMessageType::NOTE_VELOCITY || msg_type == MidiMessageType::NOTE_SWEEP) {
                midi_param = extractInt(pinConfig, "rtpNote", 60);
            } else if (msg_type == MidiMessageType::CONTROL_CHANGE) {
                midi_param = extractInt(pinConfig, "rtpCc", 7);
            } else if (msg_type == MidiMessageType::PROGRAM_CHANGE) {
                midi_param = extractInt(pinConfig, "rtpPc", 0);
            }
        }
        
        channel = extractInt(pinConfig, "rtpChan", 1);
        
        // Ajouter le composant
        ComponentType type = ComponentType::POTENTIOMETER;
        if (role == "Bouton") type = ComponentType::BUTTON;
        else if (role == "LED") type = ComponentType::LED;
        
        bool success = addComponent(gpio, type, midi_param, channel, msg_type);
        
        if (!success) {
            // Échec silencieux pour éviter le spam (les erreurs sont déjà loggées dans addComponent)
            continue;
        }
        
        // Configurer les flags OSC si le composant a été ajouté avec succès
        if (success) {
            // Trouver l'index du composant ajouté
            uint8_t index = findComponentByGpio(gpio);
            if (index != 255) {
                // Lire oscEnabled, oscFormat et oscAddress depuis la config
                bool oscEnabled = extractBool(pinConfig, "oscEnabled", false);
                String oscFormat = extractStr(pinConfig, "oscFormat", "float");
                String oscAddress = extractStr(pinConfig, "oscAddress", "");
                
                // Configurer les flags (bit 0x02 pour OSC, bit 0x04 pour format MIDI)
                if (oscEnabled) {
                    configs[index].flags |= 0x02; // Activer OSC
                    if (oscFormat == "midi") {
                        configs[index].flags |= 0x04; // Format MIDI
                    } else {
                        configs[index].flags &= ~0x04; // Format float
                    }
                } else {
                    configs[index].flags &= ~0x02; // Désactiver OSC
                }
                
                // Configurer l'adresse OSC (utiliser valeur par défaut si vide)
                if (oscAddress.length() > 0) {
                    strncpy(configs[index].osc_address, oscAddress.c_str(), sizeof(configs[index].osc_address) - 1);
                    configs[index].osc_address[sizeof(configs[index].osc_address) - 1] = '\0';
                    Serial.printf("[ComponentManager] OSC address from config: '%s' for %s\n", 
                                  oscAddress.c_str(), pinLabel.c_str());
                } else {
                    Serial.printf("[ComponentManager] OSC address empty for %s, using default: '%s'\n", 
                                  pinLabel.c_str(), configs[index].osc_address);
                }
                
                // Lire btnMode pour les boutons
                if (role == "Bouton") {
                    String btnModeStr = extractStr(pinConfig, "btnMode", "press_release");
                    if (btnModeStr.length() > 0) {
                        strncpy(configs[index].btnMode, btnModeStr.c_str(), sizeof(configs[index].btnMode) - 1);
                        configs[index].btnMode[sizeof(configs[index].btnMode) - 1] = '\0';
                    }
                    // Lire btnPulseTiming pour mode pulse
                    String btnPulseTimingStr = extractStr(pinConfig, "btnPulseTiming", "release");
                    if (btnPulseTimingStr.length() > 0) {
                        strncpy(configs[index].btnPulseTiming, btnPulseTimingStr.c_str(), sizeof(configs[index].btnPulseTiming) - 1);
                        configs[index].btnPulseTiming[sizeof(configs[index].btnPulseTiming) - 1] = '\0';
                    }
                }
                
                // Lire les paramètres pour NOTE_SWEEP (balayage)
                if (msg_type == MidiMessageType::NOTE_SWEEP) {
                    configs[index].rtpNoteMin = extractInt(pinConfig, "rtpNoteMin", 48);
                    configs[index].rtpNoteMax = extractInt(pinConfig, "rtpNoteMax", 72);
                    configs[index].rtpNoteVelFix = extractInt(pinConfig, "rtpNoteVelFix", 100);
                    configs[index].rtpNoteSweepAutoOffDelay = extractInt(pinConfig, "rtpNoteSweepAutoOffDelay", 0);
                    // S'assurer que min <= max
                    if (configs[index].rtpNoteMin > configs[index].rtpNoteMax) {
                        uint8_t temp = configs[index].rtpNoteMin;
                        configs[index].rtpNoteMin = configs[index].rtpNoteMax;
                        configs[index].rtpNoteMax = temp;
                    }
                }
                
                Serial.printf("[ComponentManager] Final OSC config: %s addr:%s for GPIO%d\n", 
                             oscEnabled ? "enabled" : "disabled", configs[index].osc_address, gpio);
            }
        }
        // Serial.printf("[ComponentManager] Added component: %s on GPIO%d -> %s\n", 
        //              pinLabel.c_str(), gpio, success ? "OK" : "FAILED");
    }
    
    preferences.end();
    // Serial.printf("[ComponentManager] Loaded %d components from NVS\n", component_count);
}

void ComponentManager::saveConfigToNVS() {
    // TODO: Implémenter la sauvegarde si nécessaire
}

const ComponentConfig* ComponentManager::getConfig(uint8_t index) const {
    if (index >= component_count) return nullptr;
    return &configs[index];
}

const ComponentState* ComponentManager::getState(uint8_t index) const {
    if (index >= component_count) return nullptr;
    return &states[index];
}

void ComponentManager::printStats() {
    Serial.println("[ComponentManager] Memory usage:");
    Serial.printf("  Configs: %d bytes (%d components)\n", component_count * sizeof(ComponentConfig), component_count);
    Serial.printf("  States: %d bytes (%d components)\n", component_count * sizeof(ComponentState), component_count);
    Serial.printf("  Filters: %d bytes (%d components)\n", component_count * sizeof(AnalogFilter), component_count);
    Serial.printf("  Total: %d bytes\n", component_count * (sizeof(ComponentConfig) + sizeof(ComponentState) + sizeof(AnalogFilter)));
    
    // Afficher les composants chargés
    for (uint8_t i = 0; i < component_count; i++) {
        const ComponentConfig& config = configs[i];
        String typeStr = "Unknown";
        switch (config.type) {
            case ComponentType::POTENTIOMETER: typeStr = "Pot"; break;
            case ComponentType::BUTTON: typeStr = "Btn"; break;
            case ComponentType::LED: typeStr = "LED"; break;
        }
        Serial.printf("  [%d] %s GPIO%d → %s %d (ch%d)\n", 
            i, typeStr.c_str(), config.gpio, 
            config.type == ComponentType::POTENTIOMETER ? "CC" : "Note",
            config.midi_param, config.midi_channel);
    }
}

// Parsing JSON optimisé
int ComponentManager::extractInt(const String& src, const char* key, int def) {
    String pat = String("\"") + key + "\":";
    int p = src.indexOf(pat);
    if (p < 0) return def;
    p += pat.length();
    
    while (p < (int)src.length() && (src[p] == ' ')) p++;
    int end = p;
    while (end < (int)src.length() && isdigit(src[end])) end++;
    
    if (end > p) return src.substring(p, end).toInt();
    return def;
}

bool ComponentManager::extractBool(const String& src, const char* key, bool def) {
    String pat = String("\"") + key + "\":";
    int p = src.indexOf(pat);
    if (p < 0) return def;
    p += pat.length();
    
    while (p < (int)src.length() && (src[p] == ' ')) p++;
    if (src.startsWith("true", p)) return true;
    if (src.startsWith("false", p)) return false;
    return def;
}

String ComponentManager::extractStr(const String& src, const char* key, const String& def) {
    String pat = String("\"") + key + "\":\"";
    int p = src.indexOf(pat);
    if (p < 0) return def;
    p += pat.length();
    
    int end = src.indexOf('"', p);
    if (end < 0) return def;
    return src.substring(p, end);
}

void ComponentManager::handleMidiNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    // Chercher les LEDs configurées pour cette note/canal
    for (uint8_t i = 0; i < component_count; i++) {
        const ComponentConfig& config = configs[i];
        if (config.type == ComponentType::LED && 
            config.midi_channel == channel && 
            config.midi_param == note) {
            
            // Allumer la LED
            digitalWrite(config.gpio, HIGH);
            // Serial.printf("[ComponentManager] LED GPIO%d ON (Note %d ch%d)\n", 
            //              config.gpio, note, channel);
        }
    }
}

void ComponentManager::handleMidiNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
    // Chercher les LEDs configurées pour cette note/canal
    for (uint8_t i = 0; i < component_count; i++) {
        const ComponentConfig& config = configs[i];
        if (config.type == ComponentType::LED && 
            config.midi_channel == channel && 
            config.midi_param == note) {
            
            // Éteindre la LED
            digitalWrite(config.gpio, LOW);
            // Serial.printf("[ComponentManager] LED GPIO%d OFF (Note %d ch%d)\n", 
            //              config.gpio, note, channel);
        }
    }
}

void ComponentManager::handleMidiControlChange(uint8_t channel, uint8_t control, uint8_t value) {
    // Chercher les LEDs configurées pour ce CC/canal
    for (uint8_t i = 0; i < component_count; i++) {
        const ComponentConfig& config = configs[i];
        if (config.type == ComponentType::LED && 
            config.midi_channel == channel && 
            config.midi_param == control) {
            
            // Allumer/éteindre selon la valeur
            bool ledState = (value > 63); // Seuil à 50%
            digitalWrite(config.gpio, ledState ? HIGH : LOW);
            // Serial.printf("[ComponentManager] LED GPIO%d %s (CC %d ch%d val%d)\n", 
            //              config.gpio, ledState ? "ON" : "OFF", control, channel, value);
        }
    }
}

