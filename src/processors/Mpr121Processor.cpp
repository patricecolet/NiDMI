#include "Mpr121Processor.h"
#include "ProcessorRegistry.h"
#include "../components/ComponentTypes.h"
#include "../components/interface/Mpr121Def.h"
#include "../hardware/Mpr121Driver.h"
#include "../midi/MidiMessageType.h"
#include <Arduino.h>

static Mpr121Driver* mpr121_driver = nullptr;
static uint8_t mpr121_last_gpio = 255;
static uint8_t mpr121_last_address = 255;
static uint8_t mpr121_last_touch_th = 0;
static uint8_t mpr121_last_release_th = 0;
static bool mpr121_logged_ok = false;
static uint32_t mpr121_last_log = 0;
static uint32_t mpr121_last_diag = 0;
static uint8_t mpr121_diag_count = 0;

void Mpr121Processor::process(
    const ComponentConfig& config,
    ComponentState& state,
    MidiSender* midi_sender,
    OSCQueue& osc_queue
) {
    if (!config.specificConfig.mpr121) {
        static unsigned long lastLog = 0;
        if (millis() - lastLog > 5000) {
            Serial.printf("[Mpr121Processor] ERREUR: specificConfig.mpr121 est NULL (GPIO %d)\n", config.gpio);
            lastLog = millis();
        }
        return;
    }

    Components::Mpr121Config* mpr121Config = config.specificConfig.mpr121;
    uint8_t i2c_addr = mpr121Config->i2c_address;
    if (i2c_addr < 90 || i2c_addr > 93) {
        i2c_addr = 90;  // 0x5A par défaut
    }
    const uint8_t addr_map[] = { 0x5A, 0x5B, 0x5C, 0x5D };
    uint8_t hardware_addr = addr_map[i2c_addr - 90];

    uint8_t touch_th = mpr121Config->touch_threshold;
    uint8_t release_th = mpr121Config->release_threshold;
    if (touch_th < 1) touch_th = 6;
    if (release_th < 1) release_th = 3;

    // (Ré)initialiser le driver si nécessaire (adresse, gpio, ou seuils changés)
    bool needInit = (mpr121_driver == nullptr)
                 || (mpr121_last_gpio != config.gpio)
                 || (mpr121_last_address != hardware_addr)
                 || (mpr121_last_touch_th != touch_th)
                 || (mpr121_last_release_th != release_th);

    if (needInit) {
        if (mpr121_driver != nullptr) {
            delete mpr121_driver;
            mpr121_driver = nullptr;
        }
        mpr121_driver = new Mpr121Driver(hardware_addr);
        mpr121_last_gpio = config.gpio;
        mpr121_last_address = hardware_addr;
        mpr121_last_touch_th = touch_th;
        mpr121_last_release_th = release_th;
        if (!mpr121_driver->begin(touch_th, release_th)) {
            Serial.printf("[Mpr121Processor] Échec init MPR121 à 0x%02X (pin bus I2C GPIO %d)\n", hardware_addr, config.gpio);
            return;
        }
        state.last_value = 0;
        state.last_raw_value_u32 = 0;
        state.last_midi_value_u8 = 0;
        state.last_telemetry_ts = 0;
        state.aux_gpio = 255;
        state.last_raw_value_aux_u32 = 0;
        state.last_midi_value_aux_u8 = 0;
        state.last_telemetry_ts_aux = 0;
        mpr121_logged_ok = false;
    }

    if (!mpr121_logged_ok) {
        Serial.printf("[Mpr121Processor] MPR121 actif: 0x%02X, note=%d, %s, touch=%d, release=%d\n",
            hardware_addr, (int)mpr121Config->base_note,
            mpr121Config->msg_type == MidiMessageType::NOTE ? "Note" : "CC",
            touch_th, release_th);
        mpr121_logged_ok = true;
        mpr121_diag_count = 0;
        mpr121_last_diag = millis();
    }

    // Diagnostic: log les 10 premières secondes après init (toutes les 2s)
    if (mpr121_diag_count < 5 && millis() - mpr121_last_diag > 2000) {
        mpr121_driver->logDiagnostic();
        mpr121_last_diag = millis();
        mpr121_diag_count++;
    }

    if (!mpr121_driver) {
        return;
    }

    uint16_t current_mask = 0;
    if (!mpr121_driver->readTouchStatus(current_mask)) {
        return;
    }

    uint16_t previous_mask = state.last_value & 0x0FFF;  // 12 bits
    bool mask_changed = (current_mask != previous_mask);
    state.last_raw_value_u32 = current_mask & 0x0FFF;   // RAW mode: mask 0..4095
    uint8_t ch = mpr121Config->midi_channel;
    if (ch < 1) ch = 1;
    if (ch > 16) ch = 16;
    uint8_t base = mpr121Config->base_note;
    MidiMessageType msg_type = mpr121Config->msg_type;

    for (uint8_t i = 0; i < 12; i++) {
        uint16_t bit = (uint16_t)1 << i;
        bool now_touched = (current_mask & bit) != 0;
        bool was_touched = (previous_mask & bit) != 0;

        if (now_touched && !was_touched) {
            // Front montant : touch
            if (millis() - mpr121_last_log > 200) {
                Serial.printf("[Mpr121Processor] Touch pad %d\n", (int)i);
                mpr121_last_log = millis();
            }
            if (msg_type == MidiMessageType::NOTE) {
                uint8_t note = base + i;
                if (note > 127) note = 127;
                if (midi_sender) {
                    midi_sender->sendNoteOn(ch, note, 100);
                }
                if (config.flags & 0x04) {  // OSC
                    char addr[40];
                    snprintf(addr, sizeof(addr), "%s/%d", config.osc_address[0] ? config.osc_address : "/mpr121", i);
                    osc_queue.enqueueMidi(addr, note, 100, ch);
                }
                // MIDI mode display: last data1 (note/CC) du dernier pad touché
                state.last_midi_value_u8 = note;
            } else {
                uint8_t cc = base + i;
                if (cc > 127) cc = 127;
                if (midi_sender) {
                    midi_sender->sendControlChange(ch, cc, 127);
                }
                if (config.flags & 0x04) {
                    char addr[40];
                    snprintf(addr, sizeof(addr), "%s/%d", config.osc_address[0] ? config.osc_address : "/mpr121", i);
                    osc_queue.enqueueMidi(addr, cc, 127, ch);
                }
                // MIDI mode display: last data1 (note/CC) du dernier pad touché
                state.last_midi_value_u8 = cc;
            }
        } else if (!now_touched && was_touched) {
            // Front descendant : release
            if (millis() - mpr121_last_log > 200) {
                Serial.printf("[Mpr121Processor] Release pad %d\n", (int)i);
                mpr121_last_log = millis();
            }
            if (msg_type == MidiMessageType::NOTE) {
                uint8_t note = base + i;
                if (note > 127) note = 127;
                if (midi_sender) {
                    midi_sender->sendNoteOff(ch, note, 0);
                }
                if (config.flags & 0x04) {
                    char addr[40];
                    snprintf(addr, sizeof(addr), "%s/%d", config.osc_address[0] ? config.osc_address : "/mpr121", i);
                    osc_queue.enqueueMidi(addr, note, 0, ch);
                }
            } else {
                uint8_t cc = base + i;
                if (cc > 127) cc = 127;
                if (midi_sender) {
                    midi_sender->sendControlChange(ch, cc, 0);
                }
                if (config.flags & 0x04) {
                    char addr[40];
                    snprintf(addr, sizeof(addr), "%s/%d", config.osc_address[0] ? config.osc_address : "/mpr121", i);
                    osc_queue.enqueueMidi(addr, cc, 0, ch);
                }
            }
        }
    }

    state.last_value = current_mask & 0x0FFF;
    if (mask_changed) {
        state.last_telemetry_ts = millis(); // LED activity: flash à chaque changement de mask
    }
}

static void processWrapper(
    const ComponentConfig& config,
    ComponentState& state,
    AnalogFilter* filter,
    MidiSender* midi_sender,
    OSCQueue& osc_queue
) {
    (void)filter;
    Mpr121Processor::process(config, state, midi_sender, osc_queue);
}

static bool registered = ProcessorRegistry::registerProcessor(
    ComponentType::MPR121,
    processWrapper
);
