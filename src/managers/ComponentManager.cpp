#include "ComponentManager.h"
#include <Arduino.h> // For Serial.printf
#include <Preferences.h>
#include "../server/ServerCore.h"
#include "../osc/OSCQueue.h"
#include "../midi/MidiMessageType.h"
#include "../config/ConfigCache.h"
#include "../config/ConfigLoader.h"
#include "../utils/JSONParser.h"
#include "../processors/ProcessorRegistry.h"
#include "../processors/Processors.h"  // Centralise tous les processeurs pour l'enregistrement automatique
#include "../components/ComponentRegistry.h"  // Pour trouver les définitions de composants
#include "../components/ValidationRegistry.h"  // Pour la validation centralisée
#include "../utils/PinMapper.h"
#include "../osc/OSCCalibrationHandler.h"
#include "../osc/OSCConfigLoader.h"
#include "../utils/ComponentInitializer.h"
#include "MuxValidator.h"
#include "../Globals.h"

ComponentManager::ComponentManager()
    : component_count(0), midi_sender(nullptr), midiTaskHandle(nullptr), midiTaskStarted(false) {
    // Initialiser les filtres
    for (int i = 0; i < MAX_COMPONENTS; i++) {
        filters[i].alpha = 0.1f;
        filters[i].initialized = false;
    }
}

ComponentManager::~ComponentManager() {
    // Arrêter la tâche MIDI avant destruction
    if (midiTaskStarted && midiTaskHandle != nullptr) {
        vTaskDelete(midiTaskHandle);
        midiTaskHandle = nullptr;
        midiTaskStarted = false;
    }
    clearAll();
}

void ComponentManager::begin(MidiSender* sender) {
    midi_sender = sender;
    /* Charger d'abord les MUX */
    loadMuxConfigFromNVS();
    /* Puis charger les configs des pins */
    ConfigLoader::loadFromNVS(*this);
    
    // Charger et initialiser la configuration OSC depuis NVS
    OSCConfigLoader::OSCConfig oscConfig = OSCConfigLoader::loadFromNVS();
    OSCConfigLoader::initialize(
        oscConfig,
        osc_manager,
        osc_queue,
        [this](const String& address, float value, const String& arg_string) {
            // D'abord, gérer les commandes de calibrage des multiplexeurs
            OSCCalibrationHandler::handleMessage(*this, address, value, arg_string);
            
            // Ensuite, router les messages OSC vers les LEDs
            LedProcessor::handleOscMessage(configs, component_count, address, value, arg_string);
        }
    );
    
    // Démarrer la tâche FreeRTOS pour les multiplexeurs
    mux_manager.begin();
    
    // Créer la tâche MIDI sur Core 0
    BaseType_t result = xTaskCreatePinnedToCore(
        midiTask,
        "MidiTask",
        8192,              // Stack size (8KB pour traitement composants)
        this,
        4,                 // Priorité légèrement inférieure à MuxTask
        &midiTaskHandle,
        0                  // Core 0 (PRO_CPU)
    );
    
    if (result == pdPASS) {
        midiTaskStarted = true;
        Serial.println("[ComponentManager] FreeRTOS MIDI task started on Core 0");
    } else {
        Serial.println("[ComponentManager] ERROR: Failed to create FreeRTOS MIDI task");
    }
    
    // Serial.printf("[ComponentManager] Loaded %d components\n", component_count);
    
    printStats();
}

void ComponentManager::syncOSCConfig() {
    // Récupérer la config de osc_manager (limiter la portée de la String)
    String targetStr = osc_manager.getTargetIP();
    int port = osc_manager.getTargetPort();
    bool broadcast = osc_manager.isBroadcastEnabled();
    
    // Appliquer à osc_queue
    osc_queue.setTarget(targetStr, port);
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
    
    // Les multiplexeurs sont maintenant lus par la tâche FreeRTOS sur Core 0
    // Seulement envoyer les batches OSC (rapide, synchrone)
    mux_manager.sendOscBatches(osc_queue);
    
    // Traiter OSC en priorité (avec queue FreeRTOS)
    osc_queue.update();
    
    // Le traitement des composants directs (non-MUX) est maintenant dans la tâche MIDI sur Core 0
    // On ne garde ici que le traitement réseau/OSC qui doit rester sur Core 1
}

void ComponentManager::reloadConfigs() {
    // Serial.println("[ComponentManager] Reloading configs...");
    clearAll();
    loadMuxConfigFromNVS();
    ConfigLoader::loadFromNVS(*this);
    // Serial.println("[ComponentManager] Configs reloaded");
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
    
    // Validation centralisée via ValidationRegistry
    const ComponentDefinition* def = ComponentRegistry::findByType(type);
    if (def && def->id) {
        // Utiliser ValidationRegistry pour valider le GPIO selon le type de composant
        if (!ValidationRegistry::validate(def->id, gpio)) {
            Serial.printf("[ComponentManager] ERROR: Invalid GPIO %d for component %s\n", gpio, def->id);
            return false;
        }
    } else {
        // Fallback si pas de définition : validation basique
        if (!is_mux_gpio && gpio > 48) {
            Serial.printf("[ComponentManager] ERROR: Invalid GPIO %d\n", gpio);
            return false;
        }
    }
    
    // Ajouter le composant
    ComponentConfig& config = configs[component_count];
    ComponentState& state = states[component_count];
    
    // Initialiser la configuration et l'état avec les valeurs par défaut
    ComponentInitializer::initializeConfig(config, gpio, type, midi_param, channel, msg_type);
    ComponentInitializer::initializeState(state);
    ComponentInitializer::setupGpio(gpio, type, &config);
    
    // Serial.printf("[ComponentManager] Added component: GPIO%d, type=%d, param=%d, channel=%d, msg_type=%d\n",
    //               gpio, (int)type, midi_param, channel, (int)msg_type);
    
    component_count++;
    return true;
}

bool ComponentManager::removeComponent(uint8_t gpio) {
    uint8_t index = findComponentByGpio(gpio);
    if (index == 255) return false;
    
    // Éteindre tous les messages MIDI actifs avant suppression
    const ComponentConfig& config = configs[index];
    ComponentState& state = states[index];
    
    if (midi_sender) {
        // 1. NOTE_SWEEP : éteindre la note active
        if (config.msg_type == MidiMessageType::NOTE_SWEEP && state.last_note != 255) {
            midi_sender->sendNoteOff(config.midi_channel, state.last_note, 0);
        }
        // 2. NOTE ou NOTE_VELOCITY : éteindre si actif
        else if (config.msg_type == MidiMessageType::NOTE || 
                 config.msg_type == MidiMessageType::NOTE_VELOCITY) {
            // Vérifier le mode du bouton
            String btnMode = String(config.btnMode);
            if (btnMode.length() == 0) {
                btnMode = "press_release"; // Défaut
            }
            
            uint8_t note = config.midi_param; // midi_param contient la note pour NOTE
            
            // Si mode toggle et état actif, envoyer Note Off
            if (btnMode == "toggle" && state.toggle_state) {
                midi_sender->sendNoteOff(config.midi_channel, note, 0);
            }
            // Si mode press_release et bouton pressé, envoyer Note Off
            else if (btnMode == "press_release" && state.prev_stable_state) {
                midi_sender->sendNoteOff(config.midi_channel, note, 0);
            }
        }
        // 3. CONTROL_CHANGE : remettre à zéro si actif
        else if (config.msg_type == MidiMessageType::CONTROL_CHANGE) {
            // Vérifier le mode du bouton (pour les boutons en CC)
            String btnMode = String(config.btnMode);
            if (btnMode.length() == 0) {
                btnMode = "press_release"; // Défaut
            }
            
            // Si mode toggle et état actif, envoyer CC=0
            if (btnMode == "toggle" && state.toggle_state) {
                midi_sender->sendControlChange(config.midi_channel, config.midi_param, 0);
            }
            // Si mode press_release et bouton pressé, envoyer CC=0
            else if (btnMode == "press_release" && state.prev_stable_state) {
                midi_sender->sendControlChange(config.midi_channel, config.midi_param, 0);
            }
            // Pour les potentiomètres en CC, si dernière valeur > 0, remettre à zéro
            // (moins critique mais peut être utile)
            else if (state.last_value > 0) {
                // Optionnel : envoyer CC=0 pour les potentiomètres
                // midi_sender->sendControlChange(config.midi_channel, config.midi_param, 0);
            }
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
    // Éteindre tous les messages MIDI actifs avant de tout effacer
    for (uint8_t i = 0; i < component_count; i++) {
        const ComponentConfig& config = configs[i];
        ComponentState& state = states[i];
        
        if (midi_sender) {
            // NOTE_SWEEP : éteindre la note active
            if (config.msg_type == MidiMessageType::NOTE_SWEEP && state.last_note != 255) {
                midi_sender->sendNoteOff(config.midi_channel, state.last_note, 0);
            }
            // NOTE ou NOTE_VELOCITY : éteindre si actif
            else if (config.msg_type == MidiMessageType::NOTE || 
                     config.msg_type == MidiMessageType::NOTE_VELOCITY) {
                String btnMode = String(config.btnMode);
                if (btnMode.length() == 0) {
                    btnMode = "press_release";
                }
                
                uint8_t note = config.midi_param;
                
                if (btnMode == "toggle" && state.toggle_state) {
                    midi_sender->sendNoteOff(config.midi_channel, note, 0);
                } else if (btnMode == "press_release" && state.prev_stable_state) {
                    midi_sender->sendNoteOff(config.midi_channel, note, 0);
                }
            }
            // CONTROL_CHANGE : remettre à zéro si actif
            else if (config.msg_type == MidiMessageType::CONTROL_CHANGE) {
                String btnMode = String(config.btnMode);
                if (btnMode.length() == 0) {
                    btnMode = "press_release";
                }
                
                if (btnMode == "toggle" && state.toggle_state) {
                    midi_sender->sendControlChange(config.midi_channel, config.midi_param, 0);
                } else if (btnMode == "press_release" && state.prev_stable_state) {
                    midi_sender->sendControlChange(config.midi_channel, config.midi_param, 0);
                }
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


void ComponentManager::saveConfigToNVS() {
    // TODO: Implémenter la sauvegarde si nécessaire
}

const ComponentConfig* ComponentManager::getConfig(uint8_t index) const {
    if (index >= component_count) return nullptr;
    return &configs[index];
}

ComponentConfig* ComponentManager::getConfigMutable(uint8_t index) {
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
        String msgTypeStr = "Note";
        
        // Utiliser ComponentDefinition pour obtenir le nom court et le type de message par défaut
        const ComponentDefinition* def = ComponentRegistry::findByType(config.type);
        if (def) {
            // Utiliser displayName ou créer un nom court depuis l'ID
            if (def->displayName) {
                // Créer un nom court (premiers 3-4 caractères)
                typeStr = String(def->displayName);
                if (typeStr.length() > 4) {
                    typeStr = typeStr.substring(0, 4);
                }
            } else if (def->id) {
                typeStr = String(def->id);
            }
            
            // Déterminer le type de message par défaut depuis le premier message MIDI
            if (def->midiMessageCount > 0 && def->midiMessages[0].id) {
                String firstMsgId = def->midiMessages[0].id;
                if (firstMsgId == "cc") {
                    msgTypeStr = "CC";
                } else if (firstMsgId == "note" || firstMsgId == "notevel" || firstMsgId == "notesweep") {
                    msgTypeStr = "Note";
                } else if (firstMsgId == "pc") {
                    msgTypeStr = "PC";
                } else {
                    msgTypeStr = "Note"; // Défaut
                }
            }
        } else {
            // Fallback : utiliser le switch case si définition non disponible
            switch (config.type) {
                case ComponentType::POTENTIOMETER: typeStr = "Pot"; msgTypeStr = "CC"; break;
                case ComponentType::BUTTON: typeStr = "Btn"; msgTypeStr = "Note"; break;
                case ComponentType::LED: typeStr = "LED"; msgTypeStr = "Note"; break;
                case ComponentType::BARGRAPH: typeStr = "Bar"; msgTypeStr = "CC"; break;
                case ComponentType::ACTUATOR: typeStr = "Act"; msgTypeStr = "Note"; break;
            }
        }
        
        Serial.printf("  [%d] %s GPIO%d → %s %d (ch%d)\n", 
            i, typeStr.c_str(), config.gpio, 
            msgTypeStr.c_str(),
            config.midi_param, config.midi_channel);
    }
}


void ComponentManager::handleMidiNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    LedProcessor::handleMidiNoteOn(configs, component_count, channel, note, velocity);
}

void ComponentManager::handleMidiNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
    LedProcessor::handleMidiNoteOff(configs, component_count, channel, note, velocity);
}

void ComponentManager::handleMidiControlChange(uint8_t channel, uint8_t control, uint8_t value) {
    LedProcessor::handleMidiControlChange(configs, component_count, channel, control, value);
}

// ============================================================================
// Gestion des multiplexeurs analogiques
// ============================================================================

bool ComponentManager::addMux(uint8_t mux_id, uint8_t sig, uint8_t s0, uint8_t s1, uint8_t s2, uint8_t s3,
                              uint8_t en, uint16_t analog_min, uint16_t analog_max,
                              bool hysteresis_enabled, MuxOSCFormat osc_format, uint8_t filter_intensity,
                              uint8_t cc_base, uint8_t midi_channel, const char* osc_base) {
    if (mux_id >= MAX_MUXES) {
        Serial.printf("[ComponentManager] Mux ID %d invalide (max %d)\n", mux_id, MAX_MUXES - 1);
        return false;
    }
    
    // Valider les pins GPIO
    MuxValidator::ValidationResult pinResult = MuxValidator::validatePins(sig, s0, s1, s2, s3, en);
    if (!pinResult.valid) {
        Serial.printf("[ComponentManager] %s\n", pinResult.error_message.c_str());
        return false;
    }
    
    // Valider les seuils
    MuxValidator::ValidationResult thresholdResult = MuxValidator::validateThresholds(analog_min, analog_max);
    if (!thresholdResult.valid) {
        Serial.printf("[ComponentManager] %s\n", thresholdResult.error_message.c_str());
        return false;
    }
    
    // Supprimer les composants existants sur les pins du MUX
    MuxValidator::removeExistingComponents(*this, sig, s0, s1, s2, s3, en);
    
    // Normaliser les paramètres MIDI
    MuxValidator::normalizeMidiParams(cc_base, midi_channel);
    
    return mux_manager.addMux(mux_id, sig, s0, s1, s2, s3, en,
                              analog_min, analog_max, hysteresis_enabled,
                              osc_format, filter_intensity, cc_base, midi_channel, osc_base);
}

bool ComponentManager::removeMux(uint8_t mux_id) {
    if (mux_id >= MAX_MUXES) return false;
    return mux_manager.removeMux(mux_id);
}

const MuxConfig* ComponentManager::getMuxConfig(uint8_t mux_id) const {
    return mux_manager.getMuxConfig(mux_id);
}

void ComponentManager::updateMuxCache(uint8_t mux_id) {
    mux_manager.updateMuxCache(mux_id);
}

bool ComponentManager::readMuxAllChannels(uint8_t mux_id, uint16_t* values) {
    return mux_manager.readMuxAllChannels(mux_id, values);
}

uint16_t ComponentManager::readMuxChannel(uint8_t gpio) {
    return mux_manager.readMuxChannel(gpio);
}

bool ComponentManager::calibrateMux(uint8_t mux_id, uint8_t channel, bool is_min, bool all_channels) {
    return mux_manager.calibrateMux(mux_id, channel, is_min, all_channels, osc_queue);
}

bool ComponentManager::resetMuxThresholds(uint8_t mux_id, uint8_t channel, bool all_channels) {
    return mux_manager.resetMuxThresholds(mux_id, channel, all_channels, osc_queue);
}

void ComponentManager::loadMuxConfigFromNVS() {
    mux_manager.loadMuxConfigFromNVS();
}

void ComponentManager::midiTask(void* parameter) {
    ComponentManager* instance = static_cast<ComponentManager*>(parameter);
    instance->midiTaskLoop();
}

void ComponentManager::midiTaskLoop() {
    const TickType_t xFrequency = pdMS_TO_TICKS(10); // 10ms
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    for(;;) {
        if (!midi_sender) {
            vTaskDelay(pdMS_TO_TICKS(100)); // Attendre si pas de sender
            continue;
        }
        
        // Envoyer les mises à jour MIDI des multiplexeurs
        mux_manager.sendMidiUpdates(midi_sender);
        
        // Traiter les composants directs (potentiomètres, boutons, etc.)
        for (uint8_t i = 0; i < component_count; i++) {
            // Vérifier que le composant est valide avant de le traiter
            const ComponentConfig& config = configs[i];
            /* Vérifier GPIO valide : 0-48 pour pins normales OU 200-247 pour MUX */
            bool is_mux_gpio = isMuxGpio(config.gpio);
            if (config.gpio >= 255 || (!is_mux_gpio && config.gpio > 48)) {
                // GPIO invalide, ignorer ce composant
                continue;
            }
            
            // Utiliser le registre de processeurs (extensible pour des centaines de composants)
            // Logique générique : si le GPIO a un ADC ou Touch, utiliser un filtre analogique
            // (indépendamment de la définition du composant - basé sur les capacités matérielles)
            const ComponentDefinition* def = ComponentRegistry::findByType(config.type);
            AnalogFilter* filter_ptr = nullptr;
            
            if (PinMapper::hasAdc(config.gpio)) {
                // GPIO avec ADC : utiliser un filtre analogique
                filter_ptr = &filters[i];
            } else if (PinMapper::hasTouch(config.gpio)) {
                // GPIO avec Touch : utiliser aussi un filtre analogique (touchRead() retourne une valeur analogique)
                filter_ptr = &filters[i];
            } else if (def && def->pinType == PinType::PIN_ANALOG) {
                // Si la définition indique PIN_ANALOG mais le GPIO n'a ni ADC ni Touch, ignorer
                continue;
            }
            
            // Appeler le processeur enregistré pour ce type de composant
            if (!ProcessorRegistry::process(config.type, configs[i], states[i], filter_ptr, midi_sender, osc_queue)) {
                // Processeur non enregistré (ne devrait pas arriver si tous les processeurs sont chargés)
                static unsigned long last_warning = 0;
                if (millis() - last_warning > 5000) {  // Limiter les warnings
                    Serial.printf("[ComponentManager] WARNING: No processor registered for component type %d (GPIO:%d)\n", 
                                 static_cast<int>(config.type), configs[i].gpio);
                    last_warning = millis();
                }
            }
        }
        
        // Attendre jusqu'à la prochaine période (10ms)
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}
