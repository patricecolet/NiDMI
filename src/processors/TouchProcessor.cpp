#include "TouchProcessor.h"
#include "ProcessorRegistry.h"
#include "../components/ComponentTypes.h"
#include "../midi/handlers/MidiOutputCoordinator.h"
#include "../utils/PinMapper.h"
#include "../config/SystemConfig.h"

// Inclure sdkconfig.h pour vérifier CONFIG_SOC_TOUCH_SENSOR_SUPPORTED
#ifdef ESP32
#include <sdkconfig.h>
#endif

// Vérifier que c'est un ESP32-S3 ET que touch sensor est supporté
#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ARDUINO_ESP32S3_DEV) || defined(ARDUINO_ESP32S3)
    #ifdef CONFIG_SOC_TOUCH_SENSOR_SUPPORTED
        #define TOUCH_AVAILABLE 1
    #else
        #define TOUCH_AVAILABLE 0
        #warning "CONFIG_SOC_TOUCH_SENSOR_SUPPORTED n'est pas activé dans sdkconfig. Ajoutez CONFIG_SOC_TOUCH_SENSOR_SUPPORTED=y dans sdkconfig.defaults"
    #endif
#else
    #define TOUCH_AVAILABLE 0
#endif

// Macro pour activer/désactiver les logs de debug
// Décommenter la ligne suivante pour activer les logs détaillés
#define DEBUG_TOUCH 0

#if DEBUG_TOUCH
    #define TOUCH_LOG(...) Serial.printf(__VA_ARGS__)
    #define TOUCH_LOG_ONCE(...) do { static bool _logged = false; if (!_logged) { Serial.printf(__VA_ARGS__); _logged = true; } } while(0)
#else
    #define TOUCH_LOG(...)
    #define TOUCH_LOG_ONCE(...)
        #endif
        
// Logs toujours actifs (essentiels)
#define TOUCH_INFO(...) Serial.printf(__VA_ARGS__)
#define TOUCH_WARN(...) Serial.printf(__VA_ARGS__)
#define TOUCH_ERROR(...) Serial.printf(__VA_ARGS__)

// ===== FONCTIONS UTILITAIRES STATIQUES =====

// Lecture tactile avec échantillonnage pour stabilité
static uint16_t readTouchValue(uint8_t gpio) {
    const int samples = 5;
        uint32_t sum = 0;
    static uint16_t last_raw_reads[49][5] = {0};
    static uint8_t sample_idx[49] = {0};
    
    for (int i = 0; i < samples; i++) {
        uint16_t raw = touchRead(gpio);
        sum += raw;
        last_raw_reads[gpio][sample_idx[gpio]] = raw;
        sample_idx[gpio] = (sample_idx[gpio] + 1) % 5;
        delayMicroseconds(200);
    }
    uint16_t avg = sum / samples;
    
    // Log détaillé des échantillons (toutes les 2 secondes)
    static unsigned long last_sample_log[49] = {0};
    if (millis() - last_sample_log[gpio] > 2000) {
        TOUCH_LOG("[TouchProcessor] GPIO%d: Échantillons [%d,%d,%d,%d,%d] → avg=%d\n",
                 gpio, last_raw_reads[gpio][0], last_raw_reads[gpio][1], 
                 last_raw_reads[gpio][2], last_raw_reads[gpio][3], last_raw_reads[gpio][4], avg);
        last_sample_log[gpio] = millis();
    }
    
    return avg;
}

// Établir la baseline pour un GPIO
static bool establishBaseline(uint8_t gpio, uint32_t& baseline) {
    static uint32_t baseline_value[49] = {0};
    static uint32_t baseline_sum[49] = {0};
    static uint8_t baseline_count[49] = {0};
    static bool baseline_set[49] = {false};
    static uint16_t baseline_min[49] = {65535};
    static uint16_t baseline_max[49] = {0};
    
    uint8_t idx = gpio;
    
    if (baseline_set[idx] && baseline_value[idx] > 0) {
        baseline = baseline_value[idx];
        TOUCH_LOG("[TouchProcessor] GPIO%d: Baseline déjà établie: %d\n", gpio, baseline);
        return true;
    }
    
    uint16_t touch_value = readTouchValue(gpio);
    baseline_sum[idx] += touch_value;
    baseline_count[idx]++;
    
    if (touch_value < baseline_min[idx]) baseline_min[idx] = touch_value;
    if (touch_value > baseline_max[idx]) baseline_max[idx] = touch_value;
    
    // Log progression de l'établissement de baseline (essentiel : toutes les 5 mesures)
    if (baseline_count[idx] <= 5 || (baseline_count[idx] % 5) == 0) {
        TOUCH_INFO("[TouchProcessor] GPIO%d: Baseline en cours... (%d/20) raw=%d, min=%d, max=%d\n",
                   gpio, baseline_count[idx], touch_value, baseline_min[idx], baseline_max[idx]);
    } else {
        TOUCH_LOG("[TouchProcessor] GPIO%d: Baseline en cours... (%d/20) raw=%d, min=%d, max=%d\n",
                  gpio, baseline_count[idx], touch_value, baseline_min[idx], baseline_max[idx]);
    }
    
    if (baseline_count[idx] >= 20) {
        baseline_value[idx] = baseline_sum[idx] / baseline_count[idx];
        baseline_set[idx] = true;
        baseline = baseline_value[idx];
        TOUCH_INFO("[TouchProcessor] ✓ Baseline GPIO%d établie: %d (min=%d, max=%d, écart=%d)\n",
                  gpio, baseline, baseline_min[idx], baseline_max[idx], 
                  baseline_max[idx] - baseline_min[idx]);
        
        if (baseline_min[idx] == baseline_max[idx]) {
            TOUCH_WARN("[TouchProcessor] ⚠️ GPIO%d: Toutes les lectures identiques (%d)! touchRead() ne change peut-être pas?\n",
                      gpio, baseline_min[idx]);
        }
        return true;
    }
    
    return false;
}

// Calculer les seuils depuis la configuration
static void calculateThresholds(
    const ComponentConfig& config,
    uint32_t baseline,
    uint32_t& touch_threshold,
    uint32_t& velocity_threshold,
    uint8_t& aftertouch_threshold
) {
    // Mapping champs génériques :
    // - customInt1 ← champ de formulaire "potMin" (seuil touch 0-4095)
    // - customInt2 ← champ de formulaire "aftertouchThreshold" (1-127)

    // Touch threshold : customInt1 (potMin) si configuré, sinon 80% de baseline
    if (config.customInt1 > 0) {
        touch_threshold = config.customInt1;
        TOUCH_LOG("[TouchProcessor] GPIO%d: touch_threshold=%d (config customInt1/potMin)\n",
                  config.gpio, touch_threshold);
    } else {
        touch_threshold = (baseline * 80) / 100;
        TOUCH_LOG("[TouchProcessor] GPIO%d: touch_threshold=%d (80%% baseline=%d)\n",
                  config.gpio, touch_threshold, baseline);
    }

    // Velocity threshold : même seuil que touch_threshold (plus de champ dédié)
    velocity_threshold = touch_threshold;
    TOUCH_LOG("[TouchProcessor] GPIO%d: velocity_threshold=%d (=touch_threshold)\n",
              config.gpio, velocity_threshold);

    // Aftertouch threshold : customInt2 (aftertouchThreshold) si configuré, sinon 4
    aftertouch_threshold = (config.customInt2 > 0) ? config.customInt2 : 4;
    TOUCH_LOG("[TouchProcessor] GPIO%d: aftertouch_threshold=%d\n", config.gpio, aftertouch_threshold);
}

// Traitement NOTE_VELOCITY
static void processNoteVelocity(
    const ComponentConfig& config,
    ComponentState& state,
    uint16_t touch_value,
    uint32_t baseline,
    uint32_t velocity_threshold,
    uint8_t aftertouch_threshold,
    MidiSender* midi_sender,
    OSCQueue& osc_queue
) {
    // Hystérésis : 2% de la baseline
    uint32_t hysteresis_margin = (baseline * 2) / 100;
        uint32_t note_on_threshold = velocity_threshold;
        uint32_t note_off_threshold = velocity_threshold + hysteresis_margin;
        
        bool note_is_on = (state.last_note != 255);
    bool is_touched = note_is_on 
        ? (touch_value < note_off_threshold)  // Note On → Note Off : seuil haut
        : (touch_value < note_on_threshold);   // Note Off → Note On : seuil bas
    
    // Calculer touch_min_value pour le mapping
        uint32_t touch_min_value;
        if (config.potMax > 0) {
        touch_min_value = config.potMax;
        } else {
        touch_min_value = (baseline * 70) / 100;
        }
        
    // Mapper vers vélocité (1-127)
        uint8_t velocity = 0;
        if (is_touched) {
            if (touch_value <= touch_min_value) {
                velocity = 127;
            } else if (touch_value >= velocity_threshold) {
                velocity = 1;
            } else {
                velocity = map(touch_value, touch_min_value, velocity_threshold, 127, 1);
                if (velocity < 1) velocity = 1;
                if (velocity > 127) velocity = 127;
            }
        }
    
    // Log continu de la valeur touch (avant traitement MIDI)
    static unsigned long last_touch_value_log = 0;
    if (millis() - last_touch_value_log > 100) {
        TOUCH_INFO("[TouchProcessor] GPIO%d: touch_value=%d (raw, avant traitement MIDI)\n", 
                  config.gpio, touch_value);
        last_touch_value_log = millis();
    }
        
        uint8_t note = config.midi_param;
        uint8_t channel = config.midi_channel;
        
    // Log détaillé de l'état et des seuils
    TOUCH_LOG("[TouchProcessor] GPIO%d: is_touched=%d, note_is_on=%d, touch_value=%d, velocity=%d, "
             "note_on_threshold=%d, note_off_threshold=%d\n",
             config.gpio, is_touched, note_is_on, touch_value, velocity, 
             note_on_threshold, note_off_threshold);
        
        if (is_touched && !note_is_on) {
        // Note On
        TOUCH_INFO("[TouchProcessor] ✓✓✓ GPIO%d → Note On (note=%d, velocity=%d, raw=%d)\n",
                         config.gpio, note, velocity, touch_value);
            if (midi_sender) {
                midi_sender->sendNoteOn(channel, note, velocity);
            TOUCH_LOG("[TouchProcessor] → MIDI Note On envoyé (ch=%d, note=%d, vel=%d)\n", 
                     channel, note, velocity);
            } else {
            TOUCH_WARN("[TouchProcessor] ⚠️ MIDI Note On NON envoyé (midi_sender=NULL)\n");
            }
            state.last_note = note;
            state.last_value = velocity;
        state.last_aftertouch = velocity;
            state.last_time = millis();
            MidiOutputCoordinator::sendOsc(osc_queue, config, velocity, touch_value);
        } else if (!is_touched && note_is_on) {
        // Note Off
        TOUCH_INFO("[TouchProcessor] ✗✗✗ GPIO%d → Note Off (note=%d, raw=%d)\n",
                         config.gpio, note, touch_value);
            if (midi_sender) {
                midi_sender->sendNoteOff(channel, note, 0);
            TOUCH_LOG("[TouchProcessor] → MIDI Note Off envoyé (ch=%d, note=%d)\n", channel, note);
            } else {
            TOUCH_WARN("[TouchProcessor] ⚠️ MIDI Note Off NON envoyé (midi_sender=NULL)\n");
            }
            state.last_note = 255;
            state.last_value = 0;
            state.last_aftertouch = 0;
            MidiOutputCoordinator::sendOsc(osc_queue, config, 0, touch_value);
        } else if (note_is_on && is_touched && velocity > 0) {
        // Key Pressure (Polyphonic Aftertouch)
        int velocity_diff = abs((int)velocity - (int)state.last_aftertouch);
            const uint32_t MIN_KEYPRESSURE_INTERVAL_MS = 20;
            uint32_t time_since_last = millis() - state.last_time;
            
        TOUCH_LOG("[TouchProcessor] GPIO%d: Key Pressure check - diff=%d (seuil=%d), time=%dms (min=%dms)\n",
                 config.gpio, velocity_diff, aftertouch_threshold, time_since_last, MIN_KEYPRESSURE_INTERVAL_MS);
        
        if (velocity_diff > aftertouch_threshold && time_since_last >= MIN_KEYPRESSURE_INTERVAL_MS) {
            TOUCH_INFO("[TouchProcessor] →→→ GPIO%d → Key Pressure (note=%d, velocity=%d, raw=%d)\n",
                             config.gpio, note, velocity, touch_value);
                if (midi_sender) {
                    midi_sender->sendKeyPressure(channel, note, velocity);
                TOUCH_LOG("[TouchProcessor] → MIDI Key Pressure envoyé (ch=%d, note=%d, vel=%d)\n", 
                         channel, note, velocity);
                } else {
                TOUCH_WARN("[TouchProcessor] ⚠️ MIDI Key Pressure NON envoyé (midi_sender=NULL)\n");
                }
                state.last_aftertouch = velocity;
                state.last_value = velocity;
                state.last_time = millis();
                MidiOutputCoordinator::sendOsc(osc_queue, config, velocity, touch_value);
            } else {
            TOUCH_LOG("[TouchProcessor] ⚠️ GPIO%d: Key Pressure bloqué - diff=%d (seuil=%d) ou time=%dms < %dms\n",
                     config.gpio, velocity_diff, aftertouch_threshold, time_since_last, MIN_KEYPRESSURE_INTERVAL_MS);
        }
    }
}

// Traitement NOTE_SWEEP
static void processNoteSweep(
    const ComponentConfig& config,
    ComponentState& state,
    uint16_t filtered_value,
    uint32_t touch_threshold,
    MidiSender* midi_sender,
    OSCQueue& osc_queue
) {
    // Auto-off
        if (config.rtpNoteSweepAutoOffDelay > 0 && 
            state.last_note != 255 && 
            state.note_on_time > 0) {
            uint32_t elapsed = millis() - state.note_on_time;
            if (elapsed >= config.rtpNoteSweepAutoOffDelay) {
                if (midi_sender) {
                    midi_sender->sendNoteOff(config.midi_channel, state.last_note, 0);
                }
                state.last_note = 255;
                state.note_on_time = 0;
            }
        }
        
    // Mapper la valeur filtrée
        uint16_t mapped_value;
        if (filtered_value >= touch_threshold) {
        mapped_value = 0;
        } else {
            mapped_value = map(filtered_value, 0, touch_threshold, 4095, 0);
        }
        
    // Hystérésis
        if (!state.hysteresis.update(mapped_value)) {
        return;
        }
        
        uint8_t stable_midi_value = state.hysteresis.getValue();
        uint8_t noteMin = config.rtpNoteMin;
        uint8_t noteMax = config.rtpNoteMax;
    uint8_t newNote = (stable_midi_value == 0) ? 255 : map(stable_midi_value, 1, 127, noteMin, noteMax);
    
        if (newNote == state.last_note) {
            return;
        }
        
        if (state.last_note != 255) {
            if (midi_sender) {
                midi_sender->sendNoteOff(config.midi_channel, state.last_note, 0);
            }
        }
        
        if (newNote != 255) {
            if (midi_sender) {
                midi_sender->sendNoteOn(config.midi_channel, newNote, config.rtpNoteVelFix);
            }
            state.note_on_time = (config.rtpNoteSweepAutoOffDelay > 0) ? millis() : 0;
        } else {
            state.note_on_time = 0;
        }
        
        state.last_note = newNote;
        state.last_value = stable_midi_value;
        state.last_time = millis();
        MidiOutputCoordinator::sendOsc(osc_queue, config, stable_midi_value, mapped_value);
}

// Traitement messages continus (CC, Pitch Bend, Aftertouch)
static void processContinuous(
    const ComponentConfig& config,
    ComponentState& state,
    uint16_t touch_value,
    uint32_t touch_threshold,
    MidiSender* midi_sender,
    OSCQueue& osc_queue
) {
    // Mapper la valeur
    uint16_t mapped_value;
    bool is_touched_cc = (touch_value < touch_threshold);
    if (touch_value >= touch_threshold) {
        mapped_value = 0;
    } else {
        mapped_value = map(touch_value, 0, touch_threshold, 4095, 0);
    }
    
    // Log continu pour CC
    static unsigned long last_cc_log[49] = {0};
    if (is_touched_cc && millis() - last_cc_log[config.gpio] > 100) {
        TOUCH_INFO("[TouchProcessor] GPIO%d: TOUCH ACTIF (CC) → raw=%d, mapped=%d, threshold=%d\n",
                         config.gpio, touch_value, mapped_value, touch_threshold);
        last_cc_log[config.gpio] = millis();
    }
    
    // Hystérésis
    if (!state.hysteresis.update(mapped_value)) {
        TOUCH_LOG("[TouchProcessor] GPIO%d: Hystérésis bloque (mapped_value=%d stable)\n",
                         config.gpio, mapped_value);
        return;
    }
    
    uint8_t midi_value = state.hysteresis.getValue();
    
    // Plage MIDI configurée
    if (config.midiCcRangeMin != 0 || config.midiCcRangeMax != 127) {
        midi_value = map(midi_value, 0, 127, config.midiCcRangeMin, config.midiCcRangeMax);
    }
    
    if (midi_value == state.last_value) {
        TOUCH_LOG("[TouchProcessor] GPIO%d: Pas de changement (midi_value=%d == last_value=%d)\n",
                         config.gpio, midi_value, state.last_value);
        return;
    }
    
    // Throttling
    const uint32_t MIN_CONTINUOUS_INTERVAL_MS = 10;
    uint32_t time_since_last = millis() - state.last_time;
    if (time_since_last < MIN_CONTINUOUS_INTERVAL_MS) {
        TOUCH_LOG("[TouchProcessor] GPIO%d: Throttling bloque (temps=%dms < min=%dms)\n",
                         config.gpio, time_since_last, MIN_CONTINUOUS_INTERVAL_MS);
        return;
    }
    
    uint16_t raw_value_for_handler = (config.msg_type == MidiMessageType::PITCH_BEND) 
        ? mapped_value 
        : mapped_value;
    
    TOUCH_INFO("[TouchProcessor] →→→ GPIO%d: CC=%d, touch_value=%d, mapped=%d, midi_value=%d\n",
              config.gpio, config.midi_param, touch_value, mapped_value, midi_value);
    
    MidiOutputCoordinator::sendMidiAndOsc(midi_sender, osc_queue, config, midi_value, raw_value_for_handler);
    state.last_value = midi_value;
    state.last_time = millis();
}

// ===== FONCTION PRINCIPALE =====

void TouchProcessor::process(
    const ComponentConfig& config,
    ComponentState& state,
    AnalogFilter& filter,
    MidiSender* midi_sender,
    OSCQueue& osc_queue
) {
    #if !TOUCH_AVAILABLE
        static unsigned long last_warning = 0;
        if (millis() - last_warning > 5000) {
            TOUCH_WARN("[TouchProcessor] WARNING: Touch non disponible sur ce MCU (ESP32-S3 requis avec CONFIG_SOC_TOUCH_SENSOR_SUPPORTED=y)\n");
            last_warning = millis();
        }
        return;
    #endif
    
    // Vérifier si touch est activé
    static bool touch_enabled_logged = false;
    bool touch_enabled = SystemConfig::isTouchEnabled();
    if (!touch_enabled_logged) {
        TOUCH_INFO("[TouchProcessor] ═══ VÉRIFICATION TOUCH ═══\n");
        TOUCH_INFO("[TouchProcessor] SystemConfig::isTouchEnabled() = %s\n", touch_enabled ? "true" : "false");
        TOUCH_INFO("[TouchProcessor] ÉTAT TOUCH: %s\n", touch_enabled ? "ACTIVÉ ✓✓✓" : "DÉSACTIVÉ ✗✗✗");
        touch_enabled_logged = true;
    }
    
    if (!touch_enabled) {
        static unsigned long last_warning = 0;
        if (millis() - last_warning > 10000) {
            TOUCH_WARN("[TouchProcessor] ⚠️⚠️⚠️ TOUCH DÉSACTIVÉ ⚠️⚠️⚠️\n");
            TOUCH_WARN("[TouchProcessor] Activez-le via l'interface web (onglet Connection -> Paramètres système)\n");
            last_warning = millis();
        }
        return;
    }
    
    // Validation GPIO
    if (config.gpio >= 255 || config.gpio > 48) {
        TOUCH_ERROR("[TouchProcessor] ERROR: GPIO invalide %d\n", config.gpio);
        return;
    }
    
    if (!PinMapper::hasTouch(config.gpio)) {
        static unsigned long last_warning = 0;
        if (millis() - last_warning > 5000) {
            TOUCH_WARN("[TouchProcessor] WARNING: GPIO%d n'a pas de capacité Touch\n", config.gpio);
            last_warning = millis();
        }
        return;
    }
    
    // Lecture tactile
    uint16_t touch_value = readTouchValue(config.gpio);
    
    // Log essentiel de la valeur brute toutes les 200ms
    static unsigned long last_raw_info[49] = {0};
    if (millis() - last_raw_info[config.gpio] > 200) {
        TOUCH_LOG("[TouchProcessor] GPIO%d: raw=%d (lecture brute)\n", config.gpio, touch_value);
        last_raw_info[config.gpio] = millis();
    }
    
    if (touch_value == 0) {
        static unsigned long last_error = 0;
        if (millis() - last_error > 5000) {
            TOUCH_ERROR("[TouchProcessor] ERROR: touchRead(GPIO%d) retourne 0 - vérifier la connexion\n", config.gpio);
            last_error = millis();
        }
        return;
    }
    
    // Configuration du filtre
    uint8_t intensity = config.filter_intensity;
    if (intensity == 0) intensity = 5;
    filter.setAlphaFromIntensity(intensity);
    
    // Filtrage
    uint16_t filtered_value = filter.process(touch_value);
    
    TOUCH_LOG("[TouchProcessor] GPIO%d: raw=%d, filtered=%d, filter_intensity=%d\n",
             config.gpio, touch_value, filtered_value, intensity);
    
    // Établir baseline
    uint32_t baseline;
    if (!establishBaseline(config.gpio, baseline)) {
        TOUCH_LOG("[TouchProcessor] GPIO%d: Baseline pas encore établie (en cours d'établissement...)\n",
                 config.gpio);
        return; // Baseline pas encore établie
    }
    
    // Calculer les seuils
    uint32_t touch_threshold;
    uint32_t velocity_threshold;
    uint8_t aftertouch_threshold;
    calculateThresholds(config, baseline, touch_threshold, velocity_threshold, aftertouch_threshold);
    
    // Log périodique des valeurs et seuils
    static unsigned long last_debug = 0;
    if (millis() - last_debug > 1000) {
        TOUCH_LOG("[TouchProcessor] GPIO%d: raw=%d, filtered=%d, baseline=%d, "
                 "touch_threshold=%d, velocity_threshold=%d\n",
                 config.gpio, touch_value, filtered_value, baseline,
                 touch_threshold, velocity_threshold);
        last_debug = millis();
    }
    
    // Traitement selon le type de message
    TOUCH_LOG_ONCE("[TouchProcessor] GPIO%d: Type message MIDI = %d\n", config.gpio, (int)config.msg_type);
    
    if (config.msg_type == MidiMessageType::NOTE_VELOCITY) {
        processNoteVelocity(config, state, touch_value, baseline, velocity_threshold, 
                          aftertouch_threshold, midi_sender, osc_queue);
    } else if (config.msg_type == MidiMessageType::NOTE_SWEEP) {
        processNoteSweep(config, state, filtered_value, touch_threshold, midi_sender, osc_queue);
    } else {
        processContinuous(config, state, touch_value, touch_threshold, midi_sender, osc_queue);
    }
}

// Wrapper pour normaliser la signature
static void processWrapper(
    const ComponentConfig& config,
    ComponentState& state,
    AnalogFilter* filter,
    MidiSender* midi_sender,
    OSCQueue& osc_queue
) {
    if (filter == nullptr) {
        return;
    }
    TouchProcessor::process(config, state, *filter, midi_sender, osc_queue);
}

// Enregistrement automatique au chargement du module
static bool registered = ProcessorRegistry::registerProcessor(
    ComponentType::TOUCH,
    processWrapper
);
