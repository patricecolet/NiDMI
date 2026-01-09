#include "ComponentManager.h"
#include <Arduino.h> // For Serial.printf
#include <Preferences.h>
#include "ServerCore.h"
#include "OSCQueue.h"
#include "midi/MidiMessageType.h"
#include "ConfigCache.h"

extern ServerCore serverCore;

ComponentManager::ComponentManager() 
    : component_count(0), midi_sender(nullptr), mux_count(0) {
    // Initialiser les filtres
    for (int i = 0; i < MAX_COMPONENTS; i++) {
        filters[i].alpha = 0.1f;
        filters[i].initialized = false;
    }
    // Initialiser les multiplexeurs
    for (int i = 0; i < MAX_MUXES; i++) {
        muxes[i] = nullptr;
        mux_configs[i].enabled = false;
    }
}

ComponentManager::~ComponentManager() {
    clearAll();
    // Libérer les multiplexeurs
    for (int i = 0; i < MAX_MUXES; i++) {
        if (muxes[i] != nullptr) {
            delete muxes[i];
            muxes[i] = nullptr;
        }
    }
}

void ComponentManager::begin(MidiSender* sender) {
    midi_sender = sender;
    /* Charger d'abord les MUX pour enregistrer les pins virtuelles dans PinMapper */
    loadMuxConfigFromNVS();
    /* Puis charger les configs des pins (y compris les pins MUX) */
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
    
    // Configurer le callback OSC pour les commandes de calibrage
    osc_manager.setMessageCallback([this](const String& address, float value, const String& arg_string) {
        Serial.printf("[ComponentManager] Callback OSC: address='%s', value=%f, arg_string='%s'\n", 
                     address.c_str(), value, arg_string.c_str());
        
        // Parser les commandes de calibrage : /mux/{id}/cal/{cmd} avec canal comme valeur
        if (address.startsWith("/mux")) {
            Serial.printf("[ComponentManager] Adresse commence par /mux\n");
            
            // Format: /mux/{id}/cal/{cmd} avec canal passé comme valeur (int) dans le message OSC
            // Si value est absent ou invalide, traiter tous les canaux
            
            int mux_id_start = address.indexOf('/', 1); // Après "/mux"
            if (mux_id_start < 0) {
                Serial.printf("[ComponentManager] ERREUR: pas de '/' après /mux\n");
                return;
            }
            
            // Extraire mux_id (peut être "0" ou "1")
            int mux_id_end = address.indexOf('/', mux_id_start + 1);
            if (mux_id_end < 0) {
                Serial.printf("[ComponentManager] ERREUR: pas de '/' après mux_id\n");
                return;
            }
            
            String mux_id_str = address.substring(mux_id_start + 1, mux_id_end);
            uint8_t mux_id = mux_id_str.toInt();
            Serial.printf("[ComponentManager] mux_id extrait: '%s' -> %d\n", mux_id_str.c_str(), mux_id);
            
            // Vérifier que mux_id est valide
            if (mux_id >= MAX_MUXES) {
                Serial.printf("[ComponentManager] ERREUR: mux_id %d >= MAX_MUXES (%d)\n", mux_id, MAX_MUXES);
                return;
            }
            
            // Vérifier que c'est une commande cal (accepter /cal ou /calibrate)
            String after_id = address.substring(mux_id_end);
            Serial.printf("[ComponentManager] Partie après mux_id: '%s', value=%f\n", 
                         after_id.c_str(), value);
            
            String cmd;
            bool all_channels = false;
            uint8_t channel = 0;
            
            // Extraire le canal depuis la valeur (value contient le numéro de canal 0-15)
            if (value >= 0 && value < 16 && (int)value == value) {
                channel = (uint8_t)value;
                all_channels = false;
            } else {
                // Pas de canal spécifié (ou invalide), traiter tous les canaux
                all_channels = true;
            }
            
            if (after_id == "/cal" || after_id == "/calibrate") {
                // Format: /mux/{id}/cal avec argument string dans le message OSC
                cmd = arg_string;
                Serial.printf("[ComponentManager] Commande extraite de l'argument: '%s'\n", cmd.c_str());
            } else if (after_id.startsWith("/cal/") || after_id.startsWith("/calibrate/")) {
                // Format: /mux/{id}/cal/{cmd} avec canal comme valeur
                int cmd_start = after_id.startsWith("/cal/") ? 5 : 12; // "/cal/" = 5, "/calibrate/" = 12
                cmd = after_id.substring(cmd_start);
                Serial.printf("[ComponentManager] Commande extraite de l'adresse: '%s'\n", cmd.c_str());
            } else {
                Serial.printf("[ComponentManager] ERREUR: ne commence pas par /cal ou /calibrate\n");
                return;
            }
            
            // Parser selon le type de commande
            if (cmd == "min") {
                if (all_channels) {
                    Serial.printf("[ComponentManager] Commande: cal/min (tous les canaux)\n");
                    this->calibrateMux(mux_id, 0, true, true);
                } else {
                    Serial.printf("[ComponentManager] Commande: cal/min canal %d\n", channel);
                    this->calibrateMux(mux_id, channel, true, false);
                }
            } else if (cmd == "max") {
                if (all_channels) {
                    Serial.printf("[ComponentManager] Commande: cal/max (tous les canaux)\n");
                    this->calibrateMux(mux_id, 0, false, true);
                } else {
                    Serial.printf("[ComponentManager] Commande: cal/max canal %d\n", channel);
                    this->calibrateMux(mux_id, channel, false, false);
                }
            } else if (cmd == "reset") {
                if (all_channels) {
                    Serial.printf("[ComponentManager] Commande: cal/reset (tous les canaux)\n");
                    this->resetMuxThresholds(mux_id, 0, true);
                } else {
                    Serial.printf("[ComponentManager] Commande: cal/reset canal %d\n", channel);
                    this->resetMuxThresholds(mux_id, channel, false);
                }
            } else {
                Serial.printf("[ComponentManager] ERREUR: commande inconnue: '%s'\n", cmd.c_str());
            }
        } else {
            Serial.printf("[ComponentManager] Adresse ne commence pas par /mux, ignorée\n");
        }
    });
    
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
    
    // Traiter les messages OSC entrants (commandes de calibrage)
    osc_manager.update();
    
    // OPTIMISATION: Mettre à jour tous les MUX en batch AVANT de traiter les composants
    for (uint8_t mux_id = 0; mux_id < MAX_MUXES; mux_id++) {
        if (muxes[mux_id] != nullptr && mux_configs[mux_id].enabled) {
            updateMuxCache(mux_id);
        }
    }
    
    // OPTIMISATION: Envoyer les batches OSC pour tous les MUX qui ont changé
    for (uint8_t mux_id = 0; mux_id < MAX_MUXES; mux_id++) {
        if (muxes[mux_id] != nullptr && mux_configs[mux_id].enabled) {
            MuxCache& cache = mux_cache[mux_id];
            
            // Envoyer le batch seulement si des valeurs ont changé
            if (cache.valid && cache.values_changed) {
                // Utiliser directement l'adresse OSC de base depuis MuxConfig
                const MuxConfig& mux_config = mux_configs[mux_id];
                String oscBase = (mux_config.osc_base[0] != '\0') ? 
                    String(mux_config.osc_base) : "/mux" + String(mux_id);
                
                // Envoyer en batch selon le format OSC configuré
                // stable_values est déjà en 0-127 (méthode Control-Surface)
                switch (mux_config.osc_format) {
                    case MuxOSCFormat::RAW: {
                        // Données brutes (0-4095) - convertir depuis 0-127 avec formule précise
                        uint16_t raw_values[16];
                        for (uint8_t ch = 0; ch < 16; ch++) {
                            raw_values[ch] = (uint16_t)cache.stable_values[ch] * 4095 / 127;  // Formule précise
                        }
                        osc_queue.enqueueIntArray(oscBase, raw_values, 16);
                        break;
                    }
                    case MuxOSCFormat::FLOAT: {
                        // Normalisé (0-1) - normaliser directement depuis 0-127
                        float normalized_values[16];
                        for (uint8_t ch = 0; ch < 16; ch++) {
                            normalized_values[ch] = cache.stable_values[ch] / 127.0f;
                        }
                        osc_queue.enqueueFloatArray(oscBase, normalized_values, 16);
                        break;
                    }
                    case MuxOSCFormat::MIDI: {
                        // MIDI standard (0-127) - utiliser directement
                        osc_queue.enqueueMidiArray(oscBase, cache.stable_values, 16);
                        break;
                    }
                }
                cache.values_changed = false; // Réinitialiser le flag
            }
        }
    }
    
    // Traiter OSC en priorité (avec queue FreeRTOS)
    osc_queue.update();
    
    for (uint8_t i = 0; i < component_count; i++) {
        // Vérifier que le composant est valide avant de le traiter
        const ComponentConfig& config = configs[i];
        /* Vérifier GPIO valide : 0-48 pour pins normales OU 200-247 pour MUX */
        bool is_mux_gpio = isMuxGpio(config.gpio);
        if (config.gpio >= 255 || (!is_mux_gpio && config.gpio > 48)) {
            // GPIO invalide, ignorer ce composant
            continue;
        }
        
        switch (config.type) {
            case ComponentType::POTENTIOMETER:
                // Vérifier ADC avant de traiter (les pins MUX ont toujours ADC)
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
    
    // Vérifier si c'est un GPIO virtuel de multiplexeur
    if (isMuxGpio(config.gpio)) {
        // Pour les MUX, utiliser directement stable_values du cache (déjà 0-127)
        // Éviter la double conversion via readMuxChannel()
        uint8_t offset = config.gpio - MUX_GPIO_BASE;
        uint8_t mux_id = offset / MUX_CHANNELS;
        uint8_t channel = offset % MUX_CHANNELS;
        
        if (mux_id >= MAX_MUXES || muxes[mux_id] == nullptr) {
            return; // Mux non configuré
        }
        
        // Mettre à jour le cache si nécessaire
        MuxCache& cache = mux_cache[mux_id];
        uint32_t now = millis();
        if (!cache.valid || (now - cache.last_update) > 10) {
            updateMuxCache(mux_id);
        }
        
        // Utiliser directement stable_values (déjà 0-127)
        uint8_t midi_value = cache.stable_values[channel];
        
        // ===== TRAITEMENT SPÉCIAL NOTE_SWEEP =====
        if (config.msg_type == MidiMessageType::NOTE_SWEEP) {
            // 1. Vérifier l'auto-off AVANT tout
            if (config.rtpNoteSweepAutoOffDelay > 0 && 
                state.last_note != 255 && 
                state.note_on_time > 0) {
                uint32_t elapsed = millis() - state.note_on_time;
                if (elapsed >= config.rtpNoteSweepAutoOffDelay) {
                    midi_sender->sendNoteOff(config.midi_channel, state.last_note, 0);
                    state.last_note = 255;
                    state.note_on_time = 0;
                }
            }
            
            // 2. Pour NOTE_SWEEP, utiliser directement la valeur (déjà stable via hystérésis globale du MUX)
            // Pas besoin d'hystérésis supplémentaire car stable_values est déjà stabilisé
            uint8_t stable_midi_value = midi_value;
        
        // 3. Calculer la nouvelle note
        uint8_t noteMin = config.rtpNoteMin;
        uint8_t noteMax = config.rtpNoteMax;
        uint8_t newNote;
        
        if (stable_midi_value == 0) {
            newNote = 255; // Pas de note (potentiomètre à zéro)
        } else {
            newNote = map(stable_midi_value, 1, 127, noteMin, noteMax);
        }
        
        // 4. Si la note est identique à la précédente, ne rien faire
        if (newNote == state.last_note) {
            return;
        }
        
        // 5. Éteindre l'ancienne note si elle existe
        if (state.last_note != 255) {
            midi_sender->sendNoteOff(config.midi_channel, state.last_note, 0);
        }
        
        // 6. Jouer la nouvelle note (sauf si 255)
        if (newNote != 255) {
            midi_sender->sendNoteOn(config.midi_channel, newNote, config.rtpNoteVelFix);
            state.note_on_time = (config.rtpNoteSweepAutoOffDelay > 0) ? millis() : 0;
        } else {
            state.note_on_time = 0;
        }
        
        // 7. Mettre à jour l'état
        state.last_note = newNote;
        state.last_value = stable_midi_value;
        state.last_time = millis();
        
        // 8. OSC si activé (même valeur que MIDI)
        if (config.flags & 0x02) {
            String oscAddress = (config.osc_address[0] != '\0') ? String(config.osc_address) : "/note";
            if (config.flags & 0x04) {
                osc_queue.enqueueMidi(oscAddress, stable_midi_value, config.midi_param, config.midi_channel);
            } else {
                osc_queue.enqueueFloat(oscAddress, stable_midi_value / 127.0f);
            }
        }
        
        return; // Traitement NOTE_SWEEP MUX terminé
    }
    
    // ===== TRAITEMENT STANDARD pour MUX (autres types) =====
    // Utiliser directement stable_values (déjà 0-127)
    // midi_value est déjà déclaré à la ligne 269
    // Envoyer seulement si changement significatif (seuil de 1 pour valeurs 0-127)
    if (abs((int)midi_value - (int)state.last_value) >= 1) {
        state.last_value = midi_value;  // Mettre à jour avant d'envoyer
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
                // Pour les MUX, l'envoi batch est géré dans update()
                // On n'envoie pas d'OSC individuel pour les MUX (envoyé en batch)
                // Les MUX sont envoyés en batch dans update()
            }
        }
        
        // Mettre à jour last_value
        state.last_value = midi_value;
        state.last_time = millis();
    }
    
    return; // Traitement MUX terminé
    }
    
    // ===== TRAITEMENT POUR GPIO NORMALES (non-MUX) =====
    // GPIO normal : vérifier qu'il est valide et a un ADC
    if (config.gpio >= 255 || config.gpio > 48) {
        return;
    }
    
    if (!PinMapper::hasAdc(config.gpio)) {
        return;
    }
    
    // Lecture analogique directe
    uint16_t raw_value = analogRead(config.gpio);
    
    // Mettre à jour alpha du filtre selon filter_intensity (1-10)
    uint8_t intensity = config.filter_intensity;
    if (intensity == 0) intensity = 5; // Valeur par défaut si non configuré
    filters[index].setAlphaFromIntensity(intensity);
    
    // Filtrage : médian + passe-bas agressif pour NOTE_SWEEP, sinon filtre normal
    uint16_t filtered_value;
    if (config.msg_type == MidiMessageType::NOTE_SWEEP) {
        filtered_value = filters[index].processMedianAndLowpass(raw_value);
    } else {
        filtered_value = filters[index].process(raw_value);
    }
    
    // ===== TRAITEMENT SPÉCIAL NOTE_SWEEP (GPIO normales) =====
    if (config.msg_type == MidiMessageType::NOTE_SWEEP) {
        // 1. Vérifier l'auto-off AVANT tout
        if (config.rtpNoteSweepAutoOffDelay > 0 && 
            state.last_note != 255 && 
            state.note_on_time > 0) {
            uint32_t elapsed = millis() - state.note_on_time;
            if (elapsed >= config.rtpNoteSweepAutoOffDelay) {
                midi_sender->sendNoteOff(config.midi_channel, state.last_note, 0);
                state.last_note = 255;
                state.note_on_time = 0;
            }
        }
        
        // 2. Appliquer l'hystérésis directement sur filtered_value (0-4095)
        // L'hystérésis réduit automatiquement vers 0-127
        if (!state.hysteresis.update(filtered_value)) {
            return; // Valeur stable, rien à faire
        }
        
        // 3. Utiliser la valeur stabilisée par l'hystérésis (déjà 0-127)
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
        
        // 9. OSC si activé (même valeur que MIDI)
        if (config.flags & 0x02) {
            String oscAddress = (config.osc_address[0] != '\0') ? String(config.osc_address) : "/note";
            if (config.flags & 0x04) {
                osc_queue.enqueueMidi(oscAddress, stable_midi_value, config.midi_param, config.midi_channel);
            } else {
                osc_queue.enqueueFloat(oscAddress, stable_midi_value / 127.0f);
            }
        }
        
        return; // Traitement NOTE_SWEEP GPIO normale terminé
    }
    
    // ===== TRAITEMENT STANDARD pour GPIO normales (autres types) =====
    // Appliquer l'hystérésis directement sur filtered_value (0-4095)
    // L'hystérésis réduit automatiquement vers 0-127 (méthode Control-Surface)
    if (!state.hysteresis.update(filtered_value)) {
        return; // Valeur stable, rien à faire
    }
    
    uint8_t midi_value = state.hysteresis.getValue();  // Déjà 0-127
    
    // Envoyer seulement si changement significatif (seuil de 1 pour valeurs 0-127)
    if (abs((int)midi_value - (int)state.last_value) >= 1) {
        state.last_value = midi_value;  // Mettre à jour avant d'envoyer
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
            case MidiMessageType::PROGRAM_CHANGE:
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
        
        // Mettre à jour last_value
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
    
    // Vérifier que le GPIO est valide (0-48 pour ESP32-C3/S3 OU 200-247 pour MUX)
    bool is_mux_gpio = isMuxGpio(gpio);
    if (gpio >= 255 || (!is_mux_gpio && gpio > 48)) {
        Serial.printf("[ComponentManager] ERROR: Invalid GPIO %d (must be 0-48 or 200-247 for MUX)\n", gpio);
        return false;
    }
    
    // Vérifier si le GPIO existe déjà
    if (findComponentByGpio(gpio) != 255) {
        Serial.printf("[ComponentManager] WARNING: GPIO %d already exists, skipping\n", gpio);
        return false;
    }
    
    // Vérifier que la pin a un ADC si c'est un potentiomètre
    if (type == ComponentType::POTENTIOMETER) {
        if (is_mux_gpio) {
            // Les pins MUX ont toujours ADC (vérifié dans hasAdc)
        } else if (!PinMapper::hasAdc(gpio)) {
            Serial.printf("[ComponentManager] ERROR: GPIO %d does not have ADC for potentiometer\n", gpio);
            return false;
        }
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
    config.filter_intensity = 5; // Défaut: filtre modéré (bon compromis)
    
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
                
                // Lire filter_intensity (1-10, défaut: 5)
                uint8_t filter_intensity = extractInt(pinConfig, "filterIntensity", 5);
                if (filter_intensity < 1) filter_intensity = 1;
                if (filter_intensity > 10) filter_intensity = 10;
                configs[index].filter_intensity = filter_intensity;
                
                Serial.printf("[ComponentManager] Final OSC config: %s addr:%s for GPIO%d\n", 
                             oscEnabled ? "enabled" : "disabled", configs[index].osc_address, gpio);
            }
        }
        // Serial.printf("[ComponentManager] Added component: %s on GPIO%d -> %s\n", 
        //              pinLabel.c_str(), gpio, success ? "OK" : "FAILED");
    }
    
    // Charger les pins MUX (M0_0 à M1_15)
    /* Ne charger que les pins MUX pour les MUX qui sont configurés */
    for (uint8_t mux_id = 0; mux_id < MAX_MUXES; mux_id++) {
        /* Vérifier si ce MUX est configuré avant de charger ses pins */
        if (!mux_configs[mux_id].enabled) {
            continue; /* MUX non configuré, ignorer ses pins */
        }
        
        for (uint8_t ch = 0; ch < MUX_CHANNELS; ch++) {
            String pinLabel = "M" + String(mux_id) + "_" + String(ch);
            String key = "pin_" + pinLabel;
            
            if (!preferences.isKey(key.c_str())) {
                continue;
            }
            
            String pinConfig = preferences.getString(key.c_str(), "");
            if (pinConfig.length() == 0) {
                continue;
            }
            
            // Parser JSON simple
            String role = extractStr(pinConfig, "role", "\n");
            if (role.length() == 0) continue;
            
            // Utiliser PinMapper pour obtenir le GPIO virtuel
            uint8_t gpio = PinMapper::labelToGpio(pinLabel);
            if (gpio == 255) {
                Serial.printf("[ComponentManager] Invalid MUX pin label: %s (GPIO=255)\n", pinLabel.c_str());
                continue;
            }
            
            // Vérifier que la pin a un ADC si c'est un potentiomètre
            if (role == "Potentiomètre") {
                if (!PinMapper::hasAdc(gpio)) {
                    Serial.printf("[ComponentManager] WARNING: MUX pin %s (GPIO%d) n'a pas d'ADC, ignorée\n", 
                                  pinLabel.c_str(), gpio);
                    continue;
                }
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
                    
                    Serial.printf("[ComponentManager] Final OSC config: %s addr:%s for MUX pin %s (GPIO%d)\n", 
                                 oscEnabled ? "enabled" : "disabled", configs[index].osc_address, pinLabel.c_str(), gpio);
                }
            }
        }
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

// ============================================================================
// Gestion des multiplexeurs analogiques
// ============================================================================

bool ComponentManager::addMux(uint8_t mux_id, uint8_t sig, uint8_t s0, uint8_t s1, uint8_t s2, uint8_t s3, 
                               uint8_t en, uint16_t analog_min, uint16_t analog_max, 
                               bool hysteresis_enabled, MuxOSCFormat osc_format, uint8_t filter_intensity,
                               const char* osc_base) {
    if (mux_id >= MAX_MUXES) {
        Serial.printf("[ComponentManager] Mux ID %d invalide (max %d)\n", mux_id, MAX_MUXES - 1);
        return false;
    }
    
    // Valider les pins GPIO (0-48 pour ESP32-C3/S3)
    if (sig > 48 || s0 > 48 || s1 > 48 || s2 > 48 || s3 > 48) {
        Serial.printf("[ComponentManager] Pin GPIO invalide (max 48)\n");
        return false;
    }
    if (en != 255 && en > 48) {
        Serial.printf("[ComponentManager] Pin EN GPIO invalide (max 48)\n");
        return false;
    }
    
    // Valider les seuils
    if (analog_min >= analog_max) {
        Serial.printf("[ComponentManager] Seuils invalides: min (%d) >= max (%d)\n", analog_min, analog_max);
        return false;
    }
    if (analog_max > 4095) {
        Serial.printf("[ComponentManager] Seuil max invalide: %d (max 4095)\n", analog_max);
        return false;
    }
    
    // Vérifier que SIG a un ADC
    if (!PinMapper::hasAdc(sig)) {
        Serial.printf("[ComponentManager] Pin SIG %d n'a pas d'ADC\n", sig);
        return false;
    }
    
    // Fonction helper : supprimer composant et NVS pour une pin
    auto removeComponentAndNVS = [this](uint8_t gpio, const char* role) {
        if (this->removeComponent(gpio)) {
            Serial.printf("[ComponentManager] Composant existant supprime de GPIO %d (utilise pour MUX %s)\n", gpio, role);
            // Supprimer aussi la configuration NVS de cette pin
            String pinLabel = PinMapper::gpioToLabel(gpio);
            if (pinLabel.length() > 0) {
                extern ConfigCache g_configCache;
                g_configCache.removeConfig(pinLabel);
            }
        }
    };
    
    // Supprimer les composants existants sur toutes les pins du multiplexeur
    removeComponentAndNVS(sig, "SIG");
    removeComponentAndNVS(s0, "S0");
    removeComponentAndNVS(s1, "S1");
    removeComponentAndNVS(s2, "S2");
    removeComponentAndNVS(s3, "S3");
    if (en != 255) {
        removeComponentAndNVS(en, "EN");
    }
    
    // Supprimer l'ancien si existe
    if (muxes[mux_id] != nullptr) {
        delete muxes[mux_id];
        muxes[mux_id] = nullptr;
    }
    
    // Créer le nouveau multiplexeur
    muxes[mux_id] = new AnalogMux(sig, s0, s1, s2, s3, en);
    if (muxes[mux_id] == nullptr) {
        Serial.printf("[ComponentManager] Echec allocation memoire pour Mux %d\n", mux_id);
        return false;
    }
    
    // Initialiser le multiplexeur
    muxes[mux_id]->begin();
    
    // Sauvegarder la config
    mux_configs[mux_id].sig_pin = sig;
    mux_configs[mux_id].s0 = s0;
    mux_configs[mux_id].s1 = s1;
    mux_configs[mux_id].s2 = s2;
    mux_configs[mux_id].s3 = s3;
    mux_configs[mux_id].en_pin = en;
    mux_configs[mux_id].enabled = true;
    // Initialiser tous les canaux avec les mêmes valeurs min/max
    for (uint8_t ch = 0; ch < 16; ch++) {
        mux_configs[mux_id].analog_min[ch] = analog_min;
        mux_configs[mux_id].analog_max[ch] = analog_max;
    }
    mux_configs[mux_id].hysteresis_enabled = hysteresis_enabled;
    mux_configs[mux_id].osc_format = osc_format;
    mux_configs[mux_id].filter_intensity = filter_intensity;
    
    // Configurer l'adresse OSC de base
    if (osc_base != nullptr && strlen(osc_base) > 0) {
        strncpy(mux_configs[mux_id].osc_base, osc_base, sizeof(mux_configs[mux_id].osc_base) - 1);
        mux_configs[mux_id].osc_base[sizeof(mux_configs[mux_id].osc_base) - 1] = '\0';
    } else {
        // Valeur par défaut : /mux{id}
        snprintf(mux_configs[mux_id].osc_base, sizeof(mux_configs[mux_id].osc_base), "/mux%d", mux_id);
    }
    
    // Compter les mux actifs
    mux_count = 0;
    for (int i = 0; i < MAX_MUXES; i++) {
        if (mux_configs[i].enabled) mux_count++;
    }
    
    // Enregistrer les pins virtuelles dans PinMapper
    PinMapper::registerMuxPins(mux_id);
    
    Serial.printf("[ComponentManager] Mux %d configure: SIG=%d, S0=%d, S1=%d, S2=%d, S3=%d, EN=%d\n",
                 mux_id, sig, s0, s1, s2, s3, en);
    
    return true;
}

bool ComponentManager::removeMux(uint8_t mux_id) {
    if (mux_id >= MAX_MUXES) return false;
    
    if (muxes[mux_id] != nullptr) {
        delete muxes[mux_id];
        muxes[mux_id] = nullptr;
    }
    
    mux_configs[mux_id].enabled = false;
    
    // Désenregistrer les pins virtuelles dans PinMapper
    PinMapper::unregisterMuxPins(mux_id);
    
    // Recompter les mux actifs
    mux_count = 0;
    for (int i = 0; i < MAX_MUXES; i++) {
        if (mux_configs[i].enabled) mux_count++;
    }
    
    Serial.printf("[ComponentManager] Mux %d supprime\n", mux_id);
    return true;
}

const MuxConfig* ComponentManager::getMuxConfig(uint8_t mux_id) const {
    if (mux_id >= MAX_MUXES) return nullptr;
    return &mux_configs[mux_id];
}

void ComponentManager::updateMuxCache(uint8_t mux_id) {
    if (mux_id >= MAX_MUXES || muxes[mux_id] == nullptr) {
        return;
    }
    
    MuxCache& cache = mux_cache[mux_id];
    const MuxConfig& config = mux_configs[mux_id];
    bool was_valid = cache.valid; // Sauvegarder l'état précédent
    
    // Lire tous les canaux en une seule passe
    if (muxes[mux_id]->readAll(cache.raw_values)) {
        // Traiter chaque canal avec seuils, filtrage, hystérésis et mapping
        cache.values_changed = false;
        for (uint8_t ch = 0; ch < 16; ch++) {
            // 1. Mapper la valeur brute depuis [analog_min, analog_max] vers [0, 4095]
            uint16_t mapped_value;
            if (config.analog_max[ch] > config.analog_min[ch]) {
                // Clamp la valeur brute dans la plage [min, max]
                int32_t raw = cache.raw_values[ch];
                int32_t min_val = config.analog_min[ch];
                int32_t max_val = config.analog_max[ch];
                
                if (raw < min_val) raw = min_val;
                if (raw > max_val) raw = max_val;
                
                // Mapper linéairement vers [0, 4095]
                // Formule: mapped = (raw - min) * 4095 / (max - min)
                mapped_value = (uint16_t)map(raw, min_val, max_val, 0, 4095);
            } else {
                // Si min == max, valeur fixe à 0
                mapped_value = 0;
            }
            
            // 2. Filtrer avec MuxChannelFilter (mettre à jour alpha selon filter_intensity)
            uint8_t intensity = config.filter_intensity;
            if (intensity == 0) intensity = 5; // Valeur par défaut si non configuré
            cache.filters[ch].setAlphaFromIntensity(intensity);
            cache.filtered_values[ch] = cache.filters[ch].process(mapped_value);
            
            // 3. Appliquer hystérésis qui réduit directement vers 7 bits (méthode Control-Surface)
            uint8_t old_stable = cache.stable_values[ch];
            if (config.hysteresis_enabled) {
                cache.hysteresis[ch].update(cache.filtered_values[ch]);
                cache.stable_values[ch] = cache.hysteresis[ch].getValue();  // Déjà 0-127
            } else {
                // Sans hystérésis, réduire directement la résolution
                cache.stable_values[ch] = cache.filtered_values[ch] >> 5;  // 12 bits → 7 bits
            }
            
            // 4. Détecter changement sur valeur stable (pas sur filtered_value)
            if (abs((int)cache.stable_values[ch] - (int)old_stable) >= 1) {
                cache.values_changed = true;
            }
        }
        cache.last_update = millis();
        cache.valid = true;
        
        // Forcer l'envoi au premier appel (quand le cache devient valide)
        if (!was_valid) {
            cache.values_changed = true;
        }
    }
}

bool ComponentManager::readMuxAllChannels(uint8_t mux_id, uint16_t* values) {
    if (mux_id >= MAX_MUXES || !values || muxes[mux_id] == nullptr) {
        return false;
    }
    
    // Mettre à jour le cache
    updateMuxCache(mux_id);
    
    // Copier les valeurs stables (avec hystérésis)
    // Convertir 0-127 vers 0-4095 pour compatibilité avec l'API
    MuxCache& cache = mux_cache[mux_id];
    for (uint8_t ch = 0; ch < 16; ch++) {
        // Convertir 7 bits → 12 bits avec formule précise : (value * 4095) / 127
        values[ch] = (uint16_t)cache.stable_values[ch] * 4095 / 127;
    }
    
    return true;
}

uint16_t ComponentManager::readMuxChannel(uint8_t gpio) {
    if (!isMuxGpio(gpio)) return 0xFFFF;
    
    // Calculer mux_id et channel depuis le GPIO virtuel
    uint8_t offset = gpio - MUX_GPIO_BASE;
    uint8_t mux_id = offset / MUX_CHANNELS;
    uint8_t channel = offset % MUX_CHANNELS;
    
    if (mux_id >= MAX_MUXES || muxes[mux_id] == nullptr) {
        return 0xFFFF; // Mux non configure
    }
    
    // Utiliser le cache si disponible et récent (< 10ms)
    MuxCache& cache = mux_cache[mux_id];
    uint32_t now = millis();
    
    if (!cache.valid || (now - cache.last_update) > 10) {
        // Cache invalide ou trop ancien, mettre à jour
        updateMuxCache(mux_id);
    }
    
    // Convertir 0-127 vers 0-4095 pour compatibilité (si utilisé ailleurs)
    // Formule précise : (value * 4095) / 127
    return (uint16_t)cache.stable_values[channel] * 4095 / 127;
}

bool ComponentManager::calibrateMux(uint8_t mux_id, uint8_t channel, bool is_min, bool all_channels) {
    if (mux_id >= MAX_MUXES || muxes[mux_id] == nullptr) {
        Serial.printf("[ComponentManager] Mux %d non configure\n", mux_id);
        return false;
    }
    
    if (!all_channels && channel >= 16) {
        Serial.printf("[ComponentManager] Canal %d invalide (max 15)\n", channel);
        return false;
    }
    
    // Mettre à jour le cache pour avoir les valeurs actuelles
    updateMuxCache(mux_id);
    MuxCache& cache = mux_cache[mux_id];
    
    if (!cache.valid) {
        Serial.printf("[ComponentManager] Cache Mux %d invalide\n", mux_id);
        return false;
    }
    
    // Calibrer selon le mode
    if (all_channels) {
        // Calibrer tous les canaux avec leurs valeurs actuelles
        for (uint8_t ch = 0; ch < 16; ch++) {
            if (is_min) {
                mux_configs[mux_id].analog_min[ch] = cache.raw_values[ch];
                Serial.printf("[ComponentManager] Mux %d canal %d: analog_min = %d\n", mux_id, ch, cache.raw_values[ch]);
            } else {
                mux_configs[mux_id].analog_max[ch] = cache.raw_values[ch];
                Serial.printf("[ComponentManager] Mux %d canal %d: analog_max = %d\n", mux_id, ch, cache.raw_values[ch]);
            }
        }
    } else {
        // Calibrer un seul canal
        if (is_min) {
            mux_configs[mux_id].analog_min[channel] = cache.raw_values[channel];
            Serial.printf("[ComponentManager] Mux %d canal %d: analog_min = %d\n", mux_id, channel, cache.raw_values[channel]);
        } else {
            mux_configs[mux_id].analog_max[channel] = cache.raw_values[channel];
            Serial.printf("[ComponentManager] Mux %d canal %d: analog_max = %d\n", mux_id, channel, cache.raw_values[channel]);
        }
    }
    
    // Sauvegarder les seuils en NVS (format binaire compact)
    Preferences prefs;
    prefs.begin("esp32server", false);
    String key = "mux_thresh_" + String(mux_id);
    
    // Vérifier si tous les canaux ont la même valeur (format compact)
    bool uniform = true;
    uint16_t first_min = mux_configs[mux_id].analog_min[0];
    uint16_t first_max = mux_configs[mux_id].analog_max[0];
    
    for (uint8_t ch = 1; ch < 16; ch++) {
        if (mux_configs[mux_id].analog_min[ch] != first_min || 
            mux_configs[mux_id].analog_max[ch] != first_max) {
            uniform = false;
            break;
        }
    }
    
    if (uniform) {
        // Format compact : 5 bytes (1 flag + 2 min + 2 max)
        uint8_t buffer[5];
        buffer[0] = 0x01; // Flag: uniform = true
        buffer[1] = first_min & 0xFF;
        buffer[2] = (first_min >> 8) & 0xFF;
        buffer[3] = first_max & 0xFF;
        buffer[4] = (first_max >> 8) & 0xFF;
        prefs.putBytes(key.c_str(), buffer, 5);
        Serial.printf("[ComponentManager] Sauvegarde seuils Mux %d (uniform: min=%d, max=%d)\n", mux_id, first_min, first_max);
    } else {
        // Format complet : 65 bytes (1 flag + 32 min + 32 max)
        uint8_t buffer[65];
        buffer[0] = 0x00; // Flag: uniform = false
        // Copier les 16 valeurs min (2 bytes chacune)
        for (uint8_t ch = 0; ch < 16; ch++) {
            uint16_t val = mux_configs[mux_id].analog_min[ch];
            buffer[1 + ch * 2] = val & 0xFF;
            buffer[1 + ch * 2 + 1] = (val >> 8) & 0xFF;
        }
        // Copier les 16 valeurs max (2 bytes chacune)
        for (uint8_t ch = 0; ch < 16; ch++) {
            uint16_t val = mux_configs[mux_id].analog_max[ch];
            buffer[33 + ch * 2] = val & 0xFF;
            buffer[33 + ch * 2 + 1] = (val >> 8) & 0xFF;
        }
        prefs.putBytes(key.c_str(), buffer, 65);
        Serial.printf("[ComponentManager] Sauvegarde seuils Mux %d (non-uniform)\n", mux_id);
    }
    
    prefs.end();
    
    // Envoyer confirmation OSC avec les valeurs stockées
    // Toujours envoyer le tableau complet, même pour un canal spécifique (comportement cohérent)
    const MuxConfig& mux_config = mux_configs[mux_id];
    String oscBase = (mux_config.osc_base[0] != '\0') ? 
        String(mux_config.osc_base) : "/mux" + String(mux_id);
    
    if (is_min) {
        osc_queue.enqueueIntArray(oscBase + "/cal/min", mux_configs[mux_id].analog_min, 16);
    } else {
        osc_queue.enqueueIntArray(oscBase + "/cal/max", mux_configs[mux_id].analog_max, 16);
    }
    
    return true;
}

bool ComponentManager::resetMuxThresholds(uint8_t mux_id, uint8_t channel, bool all_channels) {
    if (mux_id >= MAX_MUXES || muxes[mux_id] == nullptr) {
        Serial.printf("[ComponentManager] Mux %d non configure\n", mux_id);
        return false;
    }
    
    if (!all_channels && channel >= 16) {
        Serial.printf("[ComponentManager] Canal %d invalide (max 15)\n", channel);
        return false;
    }
    
    // Reset les seuils
    if (all_channels) {
        // Reset tous les canaux (0, 4095)
        for (uint8_t ch = 0; ch < 16; ch++) {
            mux_configs[mux_id].analog_min[ch] = 0;
            mux_configs[mux_id].analog_max[ch] = 4095;
        }
        Serial.printf("[ComponentManager] Reset seuils Mux %d (tous les canaux)\n", mux_id);
    } else {
        // Reset un seul canal
        mux_configs[mux_id].analog_min[channel] = 0;
        mux_configs[mux_id].analog_max[channel] = 4095;
        Serial.printf("[ComponentManager] Reset seuils Mux %d canal %d\n", mux_id, channel);
    }
    
    // Sauvegarder les seuils en NVS (format binaire compact) - même code que calibrateMux
    Preferences prefs;
    prefs.begin("esp32server", false);
    String key = "mux_thresh_" + String(mux_id);
    
    // Vérifier si tous les canaux ont la même valeur (format compact)
    bool uniform = true;
    uint16_t first_min = mux_configs[mux_id].analog_min[0];
    uint16_t first_max = mux_configs[mux_id].analog_max[0];
    
    for (uint8_t ch = 1; ch < 16; ch++) {
        if (mux_configs[mux_id].analog_min[ch] != first_min || 
            mux_configs[mux_id].analog_max[ch] != first_max) {
            uniform = false;
            break;
        }
    }
    
    if (uniform) {
        // Format compact : 5 bytes (1 flag + 2 min + 2 max)
        uint8_t buffer[5];
        buffer[0] = 0x01; // Flag: uniform = true
        buffer[1] = first_min & 0xFF;
        buffer[2] = (first_min >> 8) & 0xFF;
        buffer[3] = first_max & 0xFF;
        buffer[4] = (first_max >> 8) & 0xFF;
        prefs.putBytes(key.c_str(), buffer, 5);
    } else {
        // Format complet : 65 bytes (1 flag + 32 min + 32 max)
        uint8_t buffer[65];
        buffer[0] = 0x00; // Flag: uniform = false
        // Copier les 16 valeurs min (2 bytes chacune)
        for (uint8_t ch = 0; ch < 16; ch++) {
            uint16_t val = mux_configs[mux_id].analog_min[ch];
            buffer[1 + ch * 2] = val & 0xFF;
            buffer[1 + ch * 2 + 1] = (val >> 8) & 0xFF;
        }
        // Copier les 16 valeurs max (2 bytes chacune)
        for (uint8_t ch = 0; ch < 16; ch++) {
            uint16_t val = mux_configs[mux_id].analog_max[ch];
            buffer[33 + ch * 2] = val & 0xFF;
            buffer[33 + ch * 2 + 1] = (val >> 8) & 0xFF;
        }
        prefs.putBytes(key.c_str(), buffer, 65);
    }
    
    prefs.end();
    
    // Envoyer confirmation OSC avec les valeurs reset
    // Toujours envoyer les tableaux complets, même pour un canal spécifique (comportement cohérent)
    const MuxConfig& mux_config = mux_configs[mux_id];
    String oscBase = (mux_config.osc_base[0] != '\0') ? 
        String(mux_config.osc_base) : "/mux" + String(mux_id);
    
    // Construire les tableaux avec les valeurs actuelles (après reset)
    uint16_t min_vals[16];
    uint16_t max_vals[16];
    for (uint8_t ch = 0; ch < 16; ch++) {
        min_vals[ch] = mux_configs[mux_id].analog_min[ch];
        max_vals[ch] = mux_configs[mux_id].analog_max[ch];
    }
    osc_queue.enqueueIntArray(oscBase + "/cal/min", min_vals, 16);
    osc_queue.enqueueIntArray(oscBase + "/cal/max", max_vals, 16);
    
    return true;
}

void ComponentManager::loadMuxConfigFromNVS() {
    Preferences prefs;
    prefs.begin("esp32server", true);
    
    for (uint8_t i = 0; i < MAX_MUXES; i++) {
        String key = "mux_" + String(i);
        String config = prefs.getString(key.c_str(), "");
        
        if (!config.isEmpty()) {
            // Parser la config : "sig,s0,s1,s2,s3,en,hysteresis,osc_format,filter_intensity,osc_base" (seuils séparés)
            int vals[9] = {0, 0, 0, 0, 0, 255, 1, 1, 5}; // Défauts pour hysteresis, osc_format, filter_intensity
            String osc_base_str = "";
            int idx = 0;
            int start = 0;
            int comma_count = 0;
            
            // Compter les virgules pour savoir combien de champs on a
            for (int j = 0; j < (int)config.length(); j++) {
                if (config[j] == ',') comma_count++;
            }
            
            // Parser les 9 premiers champs (numériques)
            for (int j = 0; j <= (int)config.length() && idx < 9; j++) {
                if (j == (int)config.length() || config[j] == ',') {
                    vals[idx++] = config.substring(start, j).toInt();
                    start = j + 1;
                }
            }
            
            // Si on a 10 champs (9 virgules), le dernier est osc_base
            if (comma_count >= 9 && start < (int)config.length()) {
                osc_base_str = config.substring(start);
            }
            
            if (idx >= 5) { // Au moins sig, s0, s1, s2, s3
                // Utiliser valeurs par défaut si absentes
                bool hysteresis_enabled = (idx >= 7) ? (vals[6] != 0) : true;
                MuxOSCFormat osc_format = (idx >= 8) ? 
                    static_cast<MuxOSCFormat>(vals[7]) : MuxOSCFormat::FLOAT;
                uint8_t filter_intensity = (idx >= 9) ? vals[8] : 5;
                
                // Valider osc_format
                if (osc_format > MuxOSCFormat::MIDI) {
                    osc_format = MuxOSCFormat::FLOAT;
                }
                
                // Valider filter_intensity
                if (filter_intensity < 1) filter_intensity = 1;
                if (filter_intensity > 10) filter_intensity = 10;
                
                // Charger les seuils depuis le format binaire compact
                String thresh_key = "mux_thresh_" + String(i);
                uint8_t buffer[65];
                size_t bytes_read = prefs.getBytes(thresh_key.c_str(), buffer, 65);
                
                uint16_t analog_min = 0;
                uint16_t analog_max = 4095;
                
                if (bytes_read == 5) {
                    // Format uniform : flag + min + max
                    if (buffer[0] == 0x01) {
                        analog_min = buffer[1] | (buffer[2] << 8);
                        analog_max = buffer[3] | (buffer[4] << 8);
                    }
                } else if (bytes_read == 65) {
                    // Format non-uniform : flag + 16 min + 16 max
                    if (buffer[0] == 0x00) {
                        // Charger les premiers min/max pour initialisation (sera remplacé après)
                        analog_min = buffer[1] | (buffer[2] << 8);
                        analog_max = buffer[33] | (buffer[34] << 8);
                    }
                }
                
                // Créer le MUX avec valeurs par défaut (sera remplacé si format non-uniform)
                const char* osc_base_ptr = (osc_base_str.length() > 0) ? osc_base_str.c_str() : nullptr;
                addMux(i, vals[0], vals[1], vals[2], vals[3], vals[4], vals[5], 
                       analog_min, analog_max, hysteresis_enabled, osc_format, filter_intensity, osc_base_ptr);
                
                // Si format non-uniform, charger tous les canaux
                if (bytes_read == 65 && buffer[0] == 0x00) {
                    for (uint8_t ch = 0; ch < 16; ch++) {
                        mux_configs[i].analog_min[ch] = buffer[1 + ch * 2] | (buffer[1 + ch * 2 + 1] << 8);
                        mux_configs[i].analog_max[ch] = buffer[33 + ch * 2] | (buffer[33 + ch * 2 + 1] << 8);
                    }
                    Serial.printf("[ComponentManager] Loaded mux %d from NVS (non-uniform thresholds)\n", i);
                } else {
                    Serial.printf("[ComponentManager] Loaded mux %d from NVS (min=%d, max=%d, hyst=%d, osc_fmt=%d)\n", 
                                  i, analog_min, analog_max, hysteresis_enabled, (int)osc_format);
                }
            }
        }
    }
    
    prefs.end();
}

