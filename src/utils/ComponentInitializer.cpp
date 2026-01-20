#include "ComponentInitializer.h"
#include "../components/ComponentTypes.h"  // Définitions communes
#include "../components/ComponentRegistry.h"  // Pour obtenir les définitions
#include "../components/ComponentDefinition.h"  // Pour FormFieldDef, FieldType, MAX_FORM_FIELDS
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
    
    // Initialiser la plage MIDI (défaut: 0-127 = plage complète)
    config.midiCcRangeMin = 0;   // Défaut: 0
    config.midiCcRangeMax = 127; // Défaut: 127
    
    // Initialiser les valeurs CC On/Off pour boutons (défaut: 127 ON, 0 OFF)
    config.midiCcOnOffMin = 0;   // Défaut: 0 (OFF)
    config.midiCcOnOffMax = 127; // Défaut: 127 (ON)
    
    // Initialiser les seuils pour potentiomètre
    config.potMin = 0;    // Défaut: 0
    config.potMax = 4095; // Défaut: 4095
    
    // Initialiser les champs génériques à zéro/vide
    config.customField1[0] = '\0';
    config.customField2[0] = '\0';
    config.customInt1 = 0;
    config.customInt2 = 0;
    
    // Initialiser les champs spécifiques depuis ComponentDefinition.formFields
    const ComponentDefinition* def = ComponentRegistry::findByType(type);
    if (def && def->formFields) {
        for (uint8_t i = 0; i < def->formFieldCount && i < MAX_FORM_FIELDS; i++) {
            const FormFieldDef& field = def->formFields[i];
            if (field.id && field.defaultValue) {
                // Mapper vers les champs de ComponentConfig
                if (strcmp(field.id, "btnMode") == 0) {
                    strncpy(config.btnMode, field.defaultValue, sizeof(config.btnMode) - 1);
                    config.btnMode[sizeof(config.btnMode) - 1] = '\0';
                } else if (strcmp(field.id, "btnPulseTiming") == 0) {
                    strncpy(config.btnPulseTiming, field.defaultValue, sizeof(config.btnPulseTiming) - 1);
                    config.btnPulseTiming[sizeof(config.btnPulseTiming) - 1] = '\0';
                } else if (strcmp(field.id, "btnPullMode") == 0) {
                    strncpy(config.btnPullMode, field.defaultValue, sizeof(config.btnPullMode) - 1);
                    config.btnPullMode[sizeof(config.btnPullMode) - 1] = '\0';
                } else if (strcmp(field.id, "ledMode") == 0) {
                    strncpy(config.ledMode, field.defaultValue, sizeof(config.ledMode) - 1);
                    config.ledMode[sizeof(config.ledMode) - 1] = '\0';
                } else if (strcmp(field.id, "filterIntensity") == 0) {
                    config.filter_intensity = atoi(field.defaultValue);
                } else {
                    // Mapper vers les champs génériques pour les nouveaux composants
                    // Utiliser customField1 et customField2 pour les champs string
                    // Utiliser customInt1 et customInt2 pour les champs numériques
                    if (field.type == FieldType::TEXT || field.type == FieldType::SELECT || field.type == FieldType::CHECKBOX) {
                        if (config.customField1[0] == '\0') {
                            strncpy(config.customField1, field.defaultValue, sizeof(config.customField1) - 1);
                            config.customField1[sizeof(config.customField1) - 1] = '\0';
                        } else if (config.customField2[0] == '\0') {
                            strncpy(config.customField2, field.defaultValue, sizeof(config.customField2) - 1);
                            config.customField2[sizeof(config.customField2) - 1] = '\0';
                        }
                    } else if (field.type == FieldType::NUMBER || field.type == FieldType::RANGE) {
                        if (config.customInt1 == 0) {
                            config.customInt1 = atoi(field.defaultValue);
                        } else if (config.customInt2 == 0) {
                            config.customInt2 = atoi(field.defaultValue);
                        }
                    }
                }
            }
        }
    } else {
        // Fallback : valeurs par défaut hardcodées si ComponentDefinition non disponible
        strncpy(config.btnMode, "press_release", sizeof(config.btnMode));
        config.btnMode[sizeof(config.btnMode)-1] = '\0';
        strncpy(config.btnPulseTiming, "release", sizeof(config.btnPulseTiming));
        config.btnPulseTiming[sizeof(config.btnPulseTiming)-1] = '\0';
        strncpy(config.btnPullMode, "pullup", sizeof(config.btnPullMode));
        config.btnPullMode[sizeof(config.btnPullMode)-1] = '\0';
        strncpy(config.ledMode, "onoff", sizeof(config.ledMode));
        config.ledMode[sizeof(config.ledMode)-1] = '\0';
        config.filter_intensity = 5;
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
                if (config && strlen(config->btnPullMode) > 0) {
                    pullMode = String(config->btnPullMode);
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
