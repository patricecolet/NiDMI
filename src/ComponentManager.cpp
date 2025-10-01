#include "ComponentManager.h"
#include <Arduino.h> // For Serial.printf
#include "ServerCore.h"

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
    
    // Initialiser OSC après que le réseau/serveur soit prêt
    // Valeurs par défaut (peuvent être remplacées par la config NVS côté WebAPI)
    osc_manager.begin("255.255.255.255", 8000, 8001);
    osc_manager.setBroadcast(true);
    osc_manager.setInterface(1); // STA
    osc_manager.setEnabled(true);
    
    Serial.printf("[ComponentManager] Loaded %d components\n", component_count);
    
    printStats();
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
    
    for (uint8_t i = 0; i < component_count; i++) {
        switch (configs[i].type) {
            case ComponentType::POTENTIOMETER:
                processPotentiometer(i);
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
    clearAll();
    loadConfigFromNVS();
    Serial.println("[ComponentManager] Configs reloaded");
}

void ComponentManager::processPotentiometer(uint8_t index) {
    const ComponentConfig& config = configs[index];
    ComponentState& state = states[index];
    
    // Lecture analogique
    uint16_t raw_value = analogRead(config.gpio);
    
    // Adaptation du filtre selon la vitesse de changement
    filters[index].adaptFilter(raw_value, state.last_value);
    
    // Filtrage
    uint16_t filtered_value = filters[index].process(raw_value);
    
    // Conversion 0-4095 → 0-127
    uint8_t midi_value = map(filtered_value, 0, 4095, 0, 127);
    
    // Envoyer seulement si changement significatif (seuil de 2)
    if (abs((int)midi_value - (int)state.last_value) >= 2) {
        // Serial.printf("[ComponentManager] Potentiometer GPIO%d -> CC ch%d cc%d val%d\n", 
        //              config.gpio, config.midi_channel, config.midi_param, midi_value);
        midi_sender->sendControlChange(config.midi_channel, config.midi_param, midi_value);
        
        // Envoyer OSC si activé
        if (config.flags & 0x02) { // Bit OSC enabled
            String oscAddress = "/ctl"; // Adresse par défaut
            bool oscSent = false;
            
            if (config.flags & 0x04) { // Format MIDI
                // Envoyer 3 valeurs MIDI : valeur, numéro CC, canal
                oscSent = osc_manager.sendMidiMessage(oscAddress, midi_value, config.midi_param, config.midi_channel);
                if (oscSent) {
                    Serial.printf("[ComponentManager] OSC MIDI envoyé: %s [%d,%d,%d]\n", 
                                 oscAddress.c_str(), midi_value, config.midi_param, config.midi_channel);
                }
            } else { // Format float
                oscSent = osc_manager.sendFloat(oscAddress, midi_value / 127.0f);
                if (oscSent) {
                    Serial.printf("[ComponentManager] OSC float envoyé: %s %.3f\n", oscAddress.c_str(), midi_value / 127.0f);
                }
            }
        }
        
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
    
    // Anti-rebond simple (30ms)
    if (pressed != (bool)state.debounce_state) {
        state.last_time = now;
        state.debounce_state = pressed;
    }
    
    // Délai anti-rebond
    if (now - state.last_time > 30) {
        if (pressed && state.last_value == 0) {
            // Note On
            // Serial.printf("[ComponentManager] Button GPIO%d pressed -> Note On ch%d note%d\n", 
            //              config.gpio, config.midi_channel, config.midi_param);
            midi_sender->sendNoteOn(config.midi_channel, config.midi_param, 127);
            
            // Envoyer OSC si activé
            if (config.flags & 0x02) { // Bit OSC enabled
                String oscAddress = "/note"; // Adresse par défaut pour boutons
                bool oscSent = false;
                
                if (config.flags & 0x04) { // Format MIDI
                    // Envoyer 3 valeurs MIDI : note, vélocité, canal
                    oscSent = osc_manager.sendMidiMessage(oscAddress, config.midi_param, 127, config.midi_channel);
                    if (oscSent) {
                        Serial.printf("[ComponentManager] OSC MIDI envoyé: %s [%d,%d,%d] (bouton pressé)\n", 
                                     oscAddress.c_str(), config.midi_param, 127, config.midi_channel);
                    }
                } else { // Format float
                    oscSent = osc_manager.sendFloat(oscAddress, 1.0f); // 1.0 = pressed
                    if (oscSent) {
                        Serial.printf("[ComponentManager] OSC float envoyé: %s %.3f (bouton pressé)\n", oscAddress.c_str(), 1.0f);
                    }
                }
            }
            
            state.last_value = 127;
        } else if (!pressed && state.last_value == 127) {
            // Note Off
            // Serial.printf("[ComponentManager] Button GPIO%d released -> Note Off ch%d note%d\n", 
            //              config.gpio, config.midi_channel, config.midi_param);
            midi_sender->sendNoteOff(config.midi_channel, config.midi_param, 0);
            
            // Envoyer OSC si activé
            if (config.flags & 0x02) { // Bit OSC enabled
                String oscAddress = "/note"; // Adresse par défaut pour boutons
                bool oscSent = false;
                
                if (config.flags & 0x04) { // Format MIDI
                    // Envoyer 3 valeurs MIDI : note, vélocité, canal
                    oscSent = osc_manager.sendMidiMessage(oscAddress, config.midi_param, 0, config.midi_channel);
                    if (oscSent) {
                        Serial.printf("[ComponentManager] OSC MIDI envoyé: %s [%d,%d,%d] (bouton relâché)\n", 
                                     oscAddress.c_str(), config.midi_param, 0, config.midi_channel);
                    }
                } else { // Format float
                    oscSent = osc_manager.sendFloat(oscAddress, 0.0f); // 0.0 = released
                    if (oscSent) {
                        Serial.printf("[ComponentManager] OSC float envoyé: %s %.3f (bouton relâché)\n", oscAddress.c_str(), 0.0f);
                    }
                }
            }
            
            state.last_value = 0;
        }
    }
}

void ComponentManager::processLed(uint8_t index) {
    // Les LEDs sont pilotées par MIDI entrant
    // Cette fonction est appelée dans update() mais ne fait rien
    // Le pilotage se fait via handleMidiNoteOn/Off
}

bool ComponentManager::addComponent(uint8_t gpio, ComponentType type, uint8_t midi_param, uint8_t channel) {
    if (component_count >= MAX_COMPONENTS) return false;
    
    // Vérifier si le GPIO existe déjà
    if (findComponentByGpio(gpio) != 255) return false;
    
    // Ajouter le composant
    ComponentConfig& config = configs[component_count];
    config.gpio = gpio;
    config.type = type;
    config.midi_param = midi_param;
    config.midi_channel = channel;
    config.flags = 0x01; // rtp_enabled par défaut
    
    // Initialiser l'état
    ComponentState& state = states[component_count];
    state.last_value = 0;
    state.last_time = 0;
    state.debounce_state = 0;
    
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
    
    // Déplacer les éléments suivants
    for (uint8_t i = index; i < component_count - 1; i++) {
        configs[i] = configs[i + 1];
        states[i] = states[i + 1];
    }
    
    component_count--;
    return true;
}

void ComponentManager::clearAll() {
    component_count = 0;
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
            Serial.printf("[ComponentManager] Invalid pin label: %s\n", pinLabel.c_str());
            continue;
        }
        
        // Extraire paramètres MIDI
        uint8_t midi_param = 7; // défaut CC
        uint8_t channel = 1;    // défaut canal 1
        
        if (role == "Potentiomètre") {
            midi_param = extractInt(pinConfig, "rtpCc", 7);
        } else if (role == "Bouton") {
            midi_param = extractInt(pinConfig, "rtpNote", 60);
        }
        
        channel = extractInt(pinConfig, "rtpChan", 1);
        
        // Ajouter le composant
        ComponentType type = ComponentType::POTENTIOMETER;
        if (role == "Bouton") type = ComponentType::BUTTON;
        else if (role == "LED") type = ComponentType::LED;
        
        bool success = addComponent(gpio, type, midi_param, channel);
        
        // Configurer les flags OSC si le composant a été ajouté avec succès
        if (success) {
            // Trouver l'index du composant ajouté
            uint8_t index = findComponentByGpio(gpio);
            if (index != 255) {
                // Lire oscEnabled et oscFormat depuis la config
                bool oscEnabled = extractBool(pinConfig, "oscEnabled", false);
                String oscFormat = extractStr(pinConfig, "oscFormat", "float");
                
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
                
                Serial.printf("[ComponentManager] OSC %s (%s) pour %s (GPIO%d)\n", 
                             oscEnabled ? "activé" : "désactivé", oscFormat.c_str(), pinLabel.c_str(), gpio);
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
