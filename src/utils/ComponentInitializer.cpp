#include "ComponentInitializer.h"
#include "../components/ComponentTypes.h"  // Définitions communes
#include "../components/ComponentRegistry.h"  // Pour obtenir les définitions
#include "../components/ComponentDefinition.h"  // Pour FormFieldDef, FieldType, MAX_FORM_FIELDS
#include "../components/basic/ButtonDef.h"
#include "../components/basic/LedDef.h"
#include "../components/basic/PotentiometerDef.h"
#include "../components/basic/VelostatDef.h"
#include "../components/basic/JoystickDef.h"
#include "../components/motion/Lis3dhDef.h"
#include "../components/interface/Mpr121Def.h"
#include "../midi/MidiMessageType.h"
#include "../utils/PinMapper.h"  // Pour PinMapper::hasTouch()

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
    
    // Initialiser la plage MIDI (défaut: 0-127 = plage complète)
    config.midiCcRangeMin = 0;   // Défaut: 0
    config.midiCcRangeMax = 127; // Défaut: 127
    
    // Initialiser les valeurs CC On/Off pour boutons (défaut: 127 ON, 0 OFF)
    config.midiCcOnOffMin = 0;   // Défaut: 0 (OFF)
    config.midiCcOnOffMax = 127; // Défaut: 127 (ON)
    
    // Initialiser les champs génériques à zéro/vide
    config.customField1[0] = '\0';
    config.customField2[0] = '\0';
    config.customInt1 = 0;
    config.customInt2 = 0;
    
    // Allouer et initialiser la configuration spécifique selon le type
    const ComponentDefinition* def = ComponentRegistry::findByType(type);
    
    switch (type) {
        case ComponentType::BUTTON: {
            Components::ButtonConfig* btnConfig = new Components::ButtonConfig();
            if (def && def->formFields) {
                for (uint8_t i = 0; i < def->formFieldCount && i < MAX_FORM_FIELDS; i++) {
                    const FormFieldDef& field = def->formFields[i];
                    if (field.id && field.defaultValue) {
                        if (strcmp(field.id, "btnMode") == 0) {
                            strncpy(btnConfig->btnMode, field.defaultValue, sizeof(btnConfig->btnMode) - 1);
                            btnConfig->btnMode[sizeof(btnConfig->btnMode) - 1] = '\0';
                        } else if (strcmp(field.id, "btnPulseTiming") == 0) {
                            strncpy(btnConfig->btnPulseTiming, field.defaultValue, sizeof(btnConfig->btnPulseTiming) - 1);
                            btnConfig->btnPulseTiming[sizeof(btnConfig->btnPulseTiming) - 1] = '\0';
                        } else if (strcmp(field.id, "btnPullMode") == 0) {
                            strncpy(btnConfig->btnPullMode, field.defaultValue, sizeof(btnConfig->btnPullMode) - 1);
                            btnConfig->btnPullMode[sizeof(btnConfig->btnPullMode) - 1] = '\0';
                        }
                    }
                }
            }
            config.specificConfig.button = btnConfig;
            break;
        }
        case ComponentType::LED: {
            Components::LedConfig* ledConfig = new Components::LedConfig();
            if (def && def->formFields) {
                for (uint8_t i = 0; i < def->formFieldCount && i < MAX_FORM_FIELDS; i++) {
                    const FormFieldDef& field = def->formFields[i];
                    if (field.id && field.defaultValue && strcmp(field.id, "ledMode") == 0) {
                        strncpy(ledConfig->ledMode, field.defaultValue, sizeof(ledConfig->ledMode) - 1);
                        ledConfig->ledMode[sizeof(ledConfig->ledMode) - 1] = '\0';
                    }
                }
            }
            config.specificConfig.led = ledConfig;
            break;
        }
        case ComponentType::POTENTIOMETER: {
            Components::PotentiometerConfig* potConfig = new Components::PotentiometerConfig();
            if (def && def->formFields) {
                for (uint8_t i = 0; i < def->formFieldCount && i < MAX_FORM_FIELDS; i++) {
                    const FormFieldDef& field = def->formFields[i];
                    if (field.id && field.defaultValue) {
                        if (strcmp(field.id, "filterIntensity") == 0) {
                            potConfig->filter_intensity = atoi(field.defaultValue);
                        } else if (strcmp(field.id, "potMin") == 0) {
                            potConfig->potMin = atoi(field.defaultValue);
                        } else if (strcmp(field.id, "potMax") == 0) {
                            potConfig->potMax = atoi(field.defaultValue);
                        }
                    }
                }
            }
            config.specificConfig.potentiometer = potConfig;
            break;
        }
        case ComponentType::VELOSTAT: {
            Components::VelostatConfig* veloConfig = new Components::VelostatConfig();
            if (def && def->formFields) {
                for (uint8_t i = 0; i < def->formFieldCount && i < MAX_FORM_FIELDS; i++) {
                    const FormFieldDef& field = def->formFields[i];
                    if (field.id && field.defaultValue) {
                        if (strcmp(field.id, "filterIntensity") == 0) {
                            veloConfig->filter_intensity = atoi(field.defaultValue);
                        } else if (strcmp(field.id, "velocityThreshold") == 0) {
                            veloConfig->velocityThreshold = atoi(field.defaultValue);
                        } else if (strcmp(field.id, "aftertouchThreshold") == 0) {
                            veloConfig->aftertouchThreshold = atoi(field.defaultValue);
                        }
                    }
                }
            }
            config.specificConfig.velostat = veloConfig;
            break;
        }
        case ComponentType::JOYSTICK: {
            Components::JoystickConfig* joyConfig = new Components::JoystickConfig();
            if (def && def->formFields) {
                for (uint8_t i = 0; i < def->formFieldCount && i < MAX_FORM_FIELDS; i++) {
                    const FormFieldDef& field = def->formFields[i];
                    if (field.id && field.defaultValue) {
                        if (strcmp(field.id, "filterIntensity") == 0) {
                            joyConfig->filter_intensity = atoi(field.defaultValue);
                        } else if (strcmp(field.id, "xMin") == 0) {
                            joyConfig->joyXMin = atoi(field.defaultValue);
                        } else if (strcmp(field.id, "xZeroMin") == 0) {
                            joyConfig->joyXZeroMin = atoi(field.defaultValue);
                        } else if (strcmp(field.id, "xZeroMax") == 0) {
                            joyConfig->joyXZeroMax = atoi(field.defaultValue);
                        } else if (strcmp(field.id, "xMax") == 0) {
                            joyConfig->joyXMax = atoi(field.defaultValue);
                        } else if (strcmp(field.id, "yMin") == 0) {
                            joyConfig->joyYMin = atoi(field.defaultValue);
                        } else if (strcmp(field.id, "yZeroMin") == 0) {
                            joyConfig->joyYZeroMin = atoi(field.defaultValue);
                        } else if (strcmp(field.id, "yZeroMax") == 0) {
                            joyConfig->joyYZeroMax = atoi(field.defaultValue);
                        } else if (strcmp(field.id, "yMax") == 0) {
                            joyConfig->joyYMax = atoi(field.defaultValue);
                        }
                    }
                }
            }
            config.specificConfig.joystick = joyConfig;
            break;
        }
        case ComponentType::IMU: {
            Components::ImuConfig* imuConfig = new Components::ImuConfig();
            if (def && def->formFields) {
                for (uint8_t i = 0; i < def->formFieldCount && i < MAX_FORM_FIELDS; i++) {
                    const FormFieldDef& field = def->formFields[i];
                    if (field.id && field.defaultValue) {
                        if (strcmp(field.id, "filterIntensity") == 0) {
                            imuConfig->filter_intensity = atoi(field.defaultValue);
                        } else if (strcmp(field.id, "i2cAddress") == 0) {
                            imuConfig->i2c_address = atoi(field.defaultValue);
                        } else if (strcmp(field.id, "csGpio") == 0) {
                            imuConfig->cs_gpio = (uint8_t)atoi(field.defaultValue);
                        } else if (strcmp(field.id, "range") == 0) {
                            imuConfig->range = atoi(field.defaultValue);
                        } else if (strcmp(field.id, "dataRate") == 0) {
                            imuConfig->data_rate = atoi(field.defaultValue);
                        } else if (strcmp(field.id, "xMin") == 0) {
                            imuConfig->xMin = atoi(field.defaultValue);
                        } else if (strcmp(field.id, "xZeroMin") == 0) {
                            imuConfig->xZeroMin = atoi(field.defaultValue);
                        } else if (strcmp(field.id, "xZeroMax") == 0) {
                            imuConfig->xZeroMax = atoi(field.defaultValue);
                        } else if (strcmp(field.id, "xMax") == 0) {
                            imuConfig->xMax = atoi(field.defaultValue);
                        } else if (strcmp(field.id, "yMin") == 0) {
                            imuConfig->yMin = atoi(field.defaultValue);
                        } else if (strcmp(field.id, "yZeroMin") == 0) {
                            imuConfig->yZeroMin = atoi(field.defaultValue);
                        } else if (strcmp(field.id, "yZeroMax") == 0) {
                            imuConfig->yZeroMax = atoi(field.defaultValue);
                        } else if (strcmp(field.id, "yMax") == 0) {
                            imuConfig->yMax = atoi(field.defaultValue);
                        } else if (strcmp(field.id, "zMin") == 0) {
                            imuConfig->zMin = atoi(field.defaultValue);
                        } else if (strcmp(field.id, "zZeroMin") == 0) {
                            imuConfig->zZeroMin = atoi(field.defaultValue);
                        } else if (strcmp(field.id, "zZeroMax") == 0) {
                            imuConfig->zZeroMax = atoi(field.defaultValue);
                        } else if (strcmp(field.id, "zMax") == 0) {
                            imuConfig->zMax = atoi(field.defaultValue);
                        }
                    }
                }
            }
            config.specificConfig.imu = imuConfig;
            break;
        }
        case ComponentType::MPR121: {
            Components::Mpr121Config* mpr121Config = new Components::Mpr121Config();
            if (def && def->formFields) {
                for (uint8_t i = 0; i < def->formFieldCount && i < MAX_FORM_FIELDS; i++) {
                    const FormFieldDef& field = def->formFields[i];
                    if (field.id && field.defaultValue) {
                        if (strcmp(field.id, "i2cAddress") == 0) {
                            mpr121Config->i2c_address = (uint8_t)atoi(field.defaultValue);
                        } else if (strcmp(field.id, "baseNote") == 0) {
                            mpr121Config->base_note = (uint8_t)atoi(field.defaultValue);
                        } else if (strcmp(field.id, "touchThreshold") == 0) {
                            mpr121Config->touch_threshold = (uint8_t)atoi(field.defaultValue);
                        } else if (strcmp(field.id, "releaseThreshold") == 0) {
                            mpr121Config->release_threshold = (uint8_t)atoi(field.defaultValue);
                        }
                    }
                }
            }
            mpr121Config->midi_channel = channel;
            mpr121Config->msg_type = msg_type;
            config.specificConfig.mpr121 = mpr121Config;
            break;
        }
        default:
            // Pas de config spécifique pour ce type
            config.specificConfig.specific = nullptr;
            break;
    }
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
    
    // Pour Velostat : initialiser last_aftertouch
    state.last_aftertouch = 0;
}

void ComponentInitializer::setupGpio(uint8_t gpio, ComponentType type, const ComponentConfig* config) {
    // IMPORTANT: Ne PAS appeler pinMode() sur les pins touch ESP32-S3
    // car cela désactive la fonctionnalité touch (touchRead() ne fonctionnera plus)
    bool is_touch_type = (type == ComponentType::TOUCH);
    bool has_touch_capability = PinMapper::hasTouch(gpio);
    
    if (is_touch_type || has_touch_capability) {
        // Pour les pins touch, ne pas appeler pinMode()
        // touchRead() configure automatiquement la pin pour le touch sensing
        Serial.printf("[ComponentInitializer] ═══ GPIO%d: Touch pin détectée ═══\n", gpio);
        Serial.printf("[ComponentInitializer] Type=TOUCH: %s, hasTouch(): %s\n", 
                     is_touch_type ? "OUI" : "NON", has_touch_capability ? "OUI" : "NON");
        Serial.printf("[ComponentInitializer] ✓ Pas de pinMode() appelé (touchRead() configure automatiquement)\n");
        Serial.printf("[ComponentInitializer] ════════════════════════════════════\n");
        return;
    }
    
    const ComponentDefinition* def = ComponentRegistry::findByType(type);
    if (!def) {
        // Fallback : comportement par défaut si définition non disponible
        pinMode(gpio, INPUT);
        return;
    }
    
    // Configurer le GPIO selon le pinType de la définition
    PinType pinType = static_cast<PinType>(def->pinType);
    switch (pinType) {
        case PinType::PIN_ANALOG:
            // ADC auto, pas de configuration nécessaire
            break;
        case PinType::PIN_DIGITAL:
            // Pour les composants digitaux, déterminer INPUT ou OUTPUT selon le type
            if (type == ComponentType::BUTTON) {
                // Configurer le mode pull selon btnPullMode
                String pullMode = "pullup"; // Défaut
                if (config && config->specificConfig.button && strlen(config->specificConfig.button->btnPullMode) > 0) {
                    pullMode = String(config->specificConfig.button->btnPullMode);
                }
                
                if (pullMode == "pullup") {
                    pinMode(gpio, INPUT_PULLUP);
                } else if (pullMode == "pulldown") {
                    pinMode(gpio, INPUT_PULLDOWN);
                } else {
                    // "none" ou autre : pas de pull interne
                    pinMode(gpio, INPUT);
                }
            } else if (type == ComponentType::LED) {
                pinMode(gpio, OUTPUT);
                digitalWrite(gpio, LOW);
            } else {
                // Par défaut pour les autres composants digitaux
                pinMode(gpio, INPUT);
            }
            break;
        case PinType::PIN_ANALOG_OR_DIGITAL:
            // Utiliser comme digital par défaut (peut être changé selon le composant)
            pinMode(gpio, INPUT);
            break;
        case PinType::PIN_PWM:
            // PWM peut être INPUT ou OUTPUT selon le composant
            if (type == ComponentType::LED) {
                pinMode(gpio, OUTPUT);
                digitalWrite(gpio, LOW);
            } else {
                pinMode(gpio, INPUT);
            }
            break;
    }
}
