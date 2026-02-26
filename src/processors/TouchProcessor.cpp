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
// 1 = debug détaillé (spam), 0 = silencieux sauf WARN/ERROR
#define DEBUG_TOUCH 0

#if DEBUG_TOUCH
    #define TOUCH_LOG(...) Serial.printf(__VA_ARGS__)
    #define TOUCH_LOG_ONCE(...) do { static bool _logged = false; if (!_logged) { Serial.printf(__VA_ARGS__); _logged = true; } } while(0)
    #define TOUCH_INFO(...) Serial.printf(__VA_ARGS__)
#else
    #define TOUCH_LOG(...)
    #define TOUCH_LOG_ONCE(...)
    #define TOUCH_INFO(...)
#endif

/* Warnings et erreurs toujours actifs */
#define TOUCH_WARN(...) Serial.printf(__VA_ARGS__)
#define TOUCH_ERROR(...) Serial.printf(__VA_ARGS__)

// ===== FONCTIONS UTILITAIRES STATIQUES =====

// Lecture tactile avec échantillonnage pour stabilité (domaine brut 32 bits)
static uint32_t readTouchValue(uint8_t gpio) {
#if !TOUCH_AVAILABLE
    (void)gpio;
    return 0;
#else
    const int samples = 5;
    uint32_t sum = 0;
    static uint32_t last_raw_reads[49][5] = {0};
    static uint8_t sample_idx[49] = {0};

    for (int i = 0; i < samples; i++) {
        uint32_t raw = touchRead(gpio);
        sum += raw;
        last_raw_reads[gpio][sample_idx[gpio]] = raw;
        sample_idx[gpio] = (sample_idx[gpio] + 1) % 5;
        delayMicroseconds(200);
    }
    uint32_t avg = sum / samples;

    // Log détaillé des échantillons (toutes les 2 secondes)
    static unsigned long last_sample_log[49] = {0};
    if (millis() - last_sample_log[gpio] > 2000) {
        TOUCH_LOG("[TouchProcessor] GPIO%d: Échantillons [%lu,%lu,%lu,%lu,%lu] → avg=%lu\n",
                 gpio,
                 (unsigned long)last_raw_reads[gpio][0],
                 (unsigned long)last_raw_reads[gpio][1],
                 (unsigned long)last_raw_reads[gpio][2],
                 (unsigned long)last_raw_reads[gpio][3],
                 (unsigned long)last_raw_reads[gpio][4],
                 (unsigned long)avg);
        last_sample_log[gpio] = millis();
    }

    return avg;
#endif
}

// État global pour la baseline des GPIO tactiles
namespace {
    static uint32_t baseline_value[49]         = {0};
    static uint32_t baseline_sum[49]           = {0};
    static uint8_t  baseline_count[49]         = {0};
    static bool     baseline_set[49]           = {false};
    static uint32_t baseline_min[49]           = {UINT32_MAX};
    static uint32_t baseline_max[49]           = {0};
    static uint32_t baseline_start_time[49]    = {0};
    static uint32_t baseline_wait_last_log[49] = {0};
    static const uint32_t BASELINE_STABILIZATION_TIME_MS = 2000; // Délai pour stabilisation du signal

    void resetBaselineInternal(uint8_t idx) {
        if (idx >= 49) return;
        baseline_value[idx]         = 0;
        baseline_sum[idx]           = 0;
        baseline_count[idx]         = 0;
        baseline_set[idx]           = false;
        baseline_min[idx]           = UINT32_MAX;
        baseline_max[idx]           = 0;
        baseline_start_time[idx]    = 0;
        baseline_wait_last_log[idx] = 0;
    }
}

// Établir la baseline pour un GPIO
static bool establishBaseline(uint8_t gpio, uint32_t& baseline) {
    if (gpio > 48) {
        return false;
    }
    uint8_t idx = gpio;
    
    if (baseline_set[idx] && baseline_value[idx] > 0) {
        baseline = baseline_value[idx];
        TOUCH_LOG("[TouchProcessor] GPIO%d: Baseline déjà établie: %lu\n",
                  gpio, (unsigned long)baseline);
        return true;
    }
    
    // Démarrage du timer de stabilisation si nécessaire
    if (baseline_start_time[idx] == 0) {
        baseline_start_time[idx] = millis();
    }

    uint32_t elapsed = millis() - baseline_start_time[idx];
    if (elapsed < BASELINE_STABILIZATION_TIME_MS) {
        // On attend que le signal se stabilise avant de commencer à accumuler la baseline
        if (millis() - baseline_wait_last_log[idx] > 500) {
            TOUCH_INFO("[TouchProcessor] GPIO%d: Attente stabilisation pour baseline (%lums/%lums)\n",
                       gpio,
                       (unsigned long)elapsed,
                       (unsigned long)BASELINE_STABILIZATION_TIME_MS);
            baseline_wait_last_log[idx] = millis();
        }
        return false;
    }
    
    uint32_t touch_value = readTouchValue(gpio);
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
        baseline_set[idx]   = true;
        baseline            = baseline_value[idx];
        TOUCH_INFO("[TouchProcessor] ✓ Baseline GPIO%d établie: %lu (min=%lu, max=%lu, écart=%lu)\n",
                  gpio,
                  (unsigned long)baseline,
                  (unsigned long)baseline_min[idx],
                  (unsigned long)baseline_max[idx],
                  (unsigned long)(baseline_max[idx] - baseline_min[idx]));
        
        if (baseline_min[idx] == baseline_max[idx]) {
            TOUCH_WARN("[TouchProcessor] ⚠️ GPIO%d: Toutes les lectures identiques (%lu)! touchRead() ne change peut-être pas?\n",
                      gpio, (unsigned long)baseline_min[idx]);
        }
        return true;
    }
    
    return false;
}

// ===== MÉTHODES PUBLIQUES DE CALIBRATION =====

void TouchProcessor::resetBaseline(uint8_t gpio) {
#if !TOUCH_AVAILABLE
    (void)gpio;
    return;
#else
    if (gpio > 48) return;
    resetBaselineInternal(gpio);
#endif
}

void TouchProcessor::resetAllBaselines() {
#if !TOUCH_AVAILABLE
    return;
#else
    for (uint8_t i = 0; i < 49; ++i) {
        resetBaselineInternal(i);
    }
#endif
}

// Calculer les seuils depuis la configuration
static void calculateThresholds(
    const ComponentConfig& config,
    uint32_t baseline,
    uint32_t& touch_threshold,
    uint32_t& velocity_threshold,
    uint8_t& aftertouch_threshold
) {
    // Touch threshold : customInt1 (anciennement potMin) si configuré, sinon 80% de baseline
    if (config.customInt1 > 0) {
        touch_threshold = config.customInt1;
        TOUCH_LOG("[TouchProcessor] GPIO%d: touch_threshold=%d (config customInt1)\n", config.gpio, touch_threshold);
    } else {
        touch_threshold = (baseline * 102) / 100;
        TOUCH_LOG("[TouchProcessor] GPIO%d: touch_threshold=%d (80%% baseline=%d)\n", 
                 config.gpio, touch_threshold, baseline);
    }
    
    // Velocity threshold : customInt1 si configuré, sinon touch_threshold
    if (config.customInt1 > 0) {
        velocity_threshold = config.customInt1;
        TOUCH_LOG("[TouchProcessor] GPIO%d: velocity_threshold=%d (config customInt1)\n", 
                 config.gpio, velocity_threshold);
    } else {
        velocity_threshold = touch_threshold;
        TOUCH_LOG("[TouchProcessor] GPIO%d: velocity_threshold=%d (=touch_threshold)\n", 
                 config.gpio, velocity_threshold);
    }
    
    // Aftertouch threshold : customInt2 si configuré, sinon 4
    aftertouch_threshold = (config.customInt2 > 0) ? config.customInt2 : 4;
    TOUCH_LOG("[TouchProcessor] GPIO%d: aftertouch_threshold=%d\n", config.gpio, aftertouch_threshold);
}

// Traitement NOTE_VELOCITY
static void processNoteVelocity(
    const ComponentConfig& config,
    ComponentState& state,
    uint32_t touch_value,
    uint32_t baseline,
    uint32_t velocity_threshold,
    uint8_t aftertouch_threshold,
    MidiSender* midi_sender,
    OSCQueue& osc_queue
) {
    // Hystérésis / anti-rebond : 2% de la baseline
    // -> seuil de déclenchement (ON) plus haut que le seuil de relâchement (OFF)
    uint32_t hysteresis_margin = (baseline * 2) / 100;
    uint32_t note_off_threshold = velocity_threshold;                // seuil bas (relâchement)
    uint32_t note_on_threshold  = velocity_threshold + hysteresis_margin; // seuil haut (déclenchement)
        
    bool note_is_on = (state.last_note != 255);
    bool is_touched = note_is_on
        ? (touch_value > note_off_threshold)   // Note déjà ON → rester ON tant qu'on est au-dessus du seuil bas
        : (touch_value > note_on_threshold);   // Note OFF → déclencher uniquement au-dessus du seuil haut
    
    // Calculer touch_min_value pour le mapping
        uint32_t touch_min_value;
        if (config.customInt2 > 0) {
        touch_min_value = config.customInt2;
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
    uint32_t touch_value,
    uint32_t touch_threshold,
    MidiSender* midi_sender,
    OSCQueue& osc_queue
) {
    // Mapper la valeur (domaine brut 32 bits -> 0-4095)
    uint16_t mapped_value = 0;
    // Sur ESP32-S3, la valeur tactile MONTE quand on touche :
    // on considère donc le contact actif quand on dépasse le seuil.
    bool is_touched_cc = (touch_value > touch_threshold);

    if (is_touched_cc) {
        // Mapper la plage [touch_threshold .. max_value] vers [0 .. 4095]
        // avec saturation au‑delà de max_value pour éviter les débordements.
        uint32_t max_value = touch_threshold * 2U;
        if (touch_value >= max_value) {
            mapped_value = 4095;
        } else {
            mapped_value = map(touch_value, touch_threshold, max_value, 0, 4095);
        }
    } else {
        mapped_value = 0;
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
    
    // Lecture tactile (domaine brut 32 bits)
    uint32_t touch_raw = readTouchValue(config.gpio);
    
    // Log essentiel de la valeur brute toutes les 200ms
    static unsigned long last_raw_info[49] = {0};
    if (millis() - last_raw_info[config.gpio] > 200) {
        TOUCH_INFO("[TouchProcessor] GPIO%d: raw=%lu (lecture brute)\n",
                   config.gpio, (unsigned long)touch_raw);
        last_raw_info[config.gpio] = millis();
    }
    
    if (touch_raw == 0) {
        static unsigned long last_error = 0;
        if (millis() - last_error > 5000) {
            TOUCH_ERROR("[TouchProcessor] ERROR: touchRead(GPIO%d) retourne 0 - vérifier la connexion\n", config.gpio);
            last_error = millis();
        }
        return;
    }
    
    // Configuration du filtre (intensité 1-10)
    uint8_t intensity = 5; // Défaut
    if (config.customField1[0] != '\0') {
        int parsed = atoi(config.customField1);
        if (parsed >= 1 && parsed <= 10) {
            intensity = (uint8_t)parsed;
        }
    }
    filter.setAlphaFromIntensity(intensity);
    
    // Conversion 32 -> 16 bits pour le filtre (échelle simplifiée)
    // Ici on divise par 2 pour garder l'ordre de grandeur sans saturer trop tôt
    uint16_t touch_for_filter = (touch_raw > 131070U)
        ? 65535U
        : (uint16_t)(touch_raw / 2U);
    
    // Filtrage
    uint16_t filtered_value = filter.process(touch_for_filter);
    
    TOUCH_LOG("[TouchProcessor] GPIO%d: raw=%lu, filtered=%u, filter_intensity=%d\n",
             config.gpio, (unsigned long)touch_raw, filtered_value, intensity);
    
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
        TOUCH_LOG("[TouchProcessor] GPIO%d: raw=%lu, filtered=%u, baseline=%lu, "
                 "touch_threshold=%lu, velocity_threshold=%lu\n",
                 config.gpio,
                 (unsigned long)touch_raw,
                 filtered_value,
                 (unsigned long)baseline,
                 (unsigned long)touch_threshold,
                 (unsigned long)velocity_threshold);
        last_debug = millis();
    }
    
    // Traitement selon le type de message
    TOUCH_LOG_ONCE("[TouchProcessor] GPIO%d: Type message MIDI = %d\n", config.gpio, (int)config.msg_type);
    
    if (config.msg_type == MidiMessageType::NOTE_VELOCITY) {
        processNoteVelocity(config, state, touch_raw, baseline, velocity_threshold, 
                          aftertouch_threshold, midi_sender, osc_queue);
    } else if (config.msg_type == MidiMessageType::NOTE_SWEEP) {
        processNoteSweep(config, state, filtered_value, touch_threshold, midi_sender, osc_queue);
    } else {
        processContinuous(config, state, touch_raw, touch_threshold, midi_sender, osc_queue);
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
