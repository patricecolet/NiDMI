#include "ConfigLoader.h"
#include "../managers/ComponentManager.h"
#include "../components/ComponentTypes.h"  // Définitions communes
#include "../utils/JSONParser.h"
#include "../utils/PinMapper.h"
#include "../midi/MidiMessageType.h"
#include <Preferences.h>

void ConfigLoader::loadFromNVS(ComponentManager& manager) {
    Preferences preferences;
    preferences.begin("nidmi", true);
    
    // Serial.println("[ConfigLoader] Loading configs from NVS...");
    
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
            // Serial.printf("[ConfigLoader] Empty config for pin: %s\n", pinLabel.c_str());
            continue;
        }
        
        // Serial.printf("[ConfigLoader] Found pin: %s -> %s\n", pinLabel.c_str(), pinConfig.c_str());
        
        // Parser JSON simple
        String role = JSONParser::extractStr(pinConfig, "role", "\n");
        if (role.length() == 0) continue;
        
        // Utiliser PinMapper pour obtenir le GPIO
        uint8_t gpio = PinMapper::labelToGpio(pinLabel);
        if (gpio == 255) {
            Serial.printf("[ConfigLoader] Invalid pin label: %s (GPIO=255)\n", pinLabel.c_str());
            continue;
        }
        
        // Vérifier que la pin a un ADC si c'est un potentiomètre
        if (role == "potentiometer" || role == "Potentiomètre") {
            if (!PinMapper::hasAdc(gpio)) {
                Serial.printf("[ConfigLoader] WARNING: Pin %s (GPIO%d) n'a pas d'ADC, ignorée\n", 
                              pinLabel.c_str(), gpio);
                continue;
            }
        }
        
        // Log pour debug AVANT d'ajouter (seulement si GPIO valide)
        if (gpio < 255 && gpio <= 48) {
            // Serial.printf("[ConfigLoader] Loading pin: %s -> GPIO%d, role: %s\n", 
            //               pinLabel.c_str(), gpio, role.c_str());
        }
        
        // Extraire paramètres MIDI
        uint8_t midi_param = 7; // défaut CC
        uint8_t channel = 1;    // défaut canal 1
        MidiMessageType msg_type = MidiMessageType::NOTE; // défaut
        
        // Lire rtpType depuis la config
        String rtpTypeStr = JSONParser::extractStr(pinConfig, "rtpType", "");
        if (rtpTypeStr.length() > 0) {
            msg_type = stringToMidiMessageType(rtpTypeStr);
        } else {
            // Défaut selon le rôle si rtpType n'est pas spécifié
            if (role == "potentiometer" || role == "Potentiomètre") {
                msg_type = MidiMessageType::CONTROL_CHANGE;
            } else if (role == "button" || role == "Bouton") {
                msg_type = MidiMessageType::NOTE;
            }
        }
        
        // Extraire le paramètre MIDI selon le type de message
        if (role == "potentiometer" || role == "Potentiomètre") {
            if (msg_type == MidiMessageType::CONTROL_CHANGE) {
                midi_param = JSONParser::extractInt(pinConfig, "rtpCc", 7);
            } else if (msg_type == MidiMessageType::PROGRAM_CHANGE) {
                midi_param = JSONParser::extractInt(pinConfig, "rtpPc", 0);
            } else if (msg_type == MidiMessageType::NOTE || msg_type == MidiMessageType::NOTE_VELOCITY || msg_type == MidiMessageType::NOTE_SWEEP) {
                midi_param = JSONParser::extractInt(pinConfig, "rtpNote", 60);
            }
        } else if (role == "button" || role == "Bouton") {
            if (msg_type == MidiMessageType::NOTE || msg_type == MidiMessageType::NOTE_VELOCITY || msg_type == MidiMessageType::NOTE_SWEEP) {
                midi_param = JSONParser::extractInt(pinConfig, "rtpNote", 60);
            } else if (msg_type == MidiMessageType::CONTROL_CHANGE) {
                midi_param = JSONParser::extractInt(pinConfig, "rtpCc", 7);
            } else if (msg_type == MidiMessageType::PROGRAM_CHANGE) {
                midi_param = JSONParser::extractInt(pinConfig, "rtpPc", 0);
            }
        }
        
        channel = JSONParser::extractInt(pinConfig, "rtpChan", 1);
        
        // Ajouter le composant
        ComponentType type = ComponentType::POTENTIOMETER;
        if (role == "Bouton") type = ComponentType::BUTTON;
        else if (role == "LED") type = ComponentType::LED;
        
        bool success = manager.addComponent(gpio, type, midi_param, channel, msg_type);
        
        if (!success) {
            // Échec silencieux pour éviter le spam (les erreurs sont déjà loggées dans addComponent)
            continue;
        }
        
        // Configurer les flags OSC si le composant a été ajouté avec succès
        if (success) {
            // Trouver l'index du composant ajouté
            uint8_t index = manager.findComponentByGpio(gpio);
            if (index != 255) {
                ComponentConfig* config = manager.getConfigMutable(index);
                if (config) {
                    // Lire oscEnabled, oscFormat et oscAddress depuis la config
                    bool oscEnabled = JSONParser::extractBool(pinConfig, "oscEnabled", false);
                    String oscFormat = JSONParser::extractStr(pinConfig, "oscFormat", "float");
                    String oscAddress = JSONParser::extractStr(pinConfig, "oscAddress", "");
                    
                    // Configurer les flags (bit 0x02 pour OSC, bit 0x04 pour format MIDI)
                    if (oscEnabled) {
                        config->flags |= 0x02; // Activer OSC
                        if (oscFormat == "midi") {
                            config->flags |= 0x04; // Format MIDI
                        } else {
                            config->flags &= ~0x04; // Format float
                        }
                    } else {
                        config->flags &= ~0x02; // Désactiver OSC
                    }
                    
                    // Configurer l'adresse OSC (utiliser valeur par défaut si vide)
                    if (oscAddress.length() > 0) {
                        strncpy(config->osc_address, oscAddress.c_str(), sizeof(config->osc_address) - 1);
                        config->osc_address[sizeof(config->osc_address) - 1] = '\0';
                        Serial.printf("[ConfigLoader] OSC address from config: '%s' for %s\n", 
                                      oscAddress.c_str(), pinLabel.c_str());
                    } else {
                        Serial.printf("[ConfigLoader] OSC address empty for %s, using default: '%s'\n", 
                                      pinLabel.c_str(), config->osc_address);
                    }
                    
                    // Lire btnMode pour les boutons
                    if (role == "Bouton") {
                        String btnModeStr = JSONParser::extractStr(pinConfig, "btnMode", "press_release");
                        if (btnModeStr.length() > 0) {
                            strncpy(config->btnMode, btnModeStr.c_str(), sizeof(config->btnMode) - 1);
                            config->btnMode[sizeof(config->btnMode) - 1] = '\0';
                        }
                        // Lire btnPulseTiming pour mode pulse
                        String btnPulseTimingStr = JSONParser::extractStr(pinConfig, "btnPulseTiming", "release");
                        if (btnPulseTimingStr.length() > 0) {
                            strncpy(config->btnPulseTiming, btnPulseTimingStr.c_str(), sizeof(config->btnPulseTiming) - 1);
                            config->btnPulseTiming[sizeof(config->btnPulseTiming) - 1] = '\0';
                        }
                    }
                    
                    // Lire les paramètres pour NOTE_SWEEP (balayage)
                    if (msg_type == MidiMessageType::NOTE_SWEEP) {
                        config->rtpNoteMin = JSONParser::extractInt(pinConfig, "rtpNoteMin", 48);
                        config->rtpNoteMax = JSONParser::extractInt(pinConfig, "rtpNoteMax", 72);
                        config->rtpNoteVelFix = JSONParser::extractInt(pinConfig, "rtpNoteVelFix", 100);
                        config->rtpNoteSweepAutoOffDelay = JSONParser::extractInt(pinConfig, "rtpNoteSweepAutoOffDelay", 0);
                        // S'assurer que min <= max
                        if (config->rtpNoteMin > config->rtpNoteMax) {
                            uint8_t temp = config->rtpNoteMin;
                            config->rtpNoteMin = config->rtpNoteMax;
                            config->rtpNoteMax = temp;
                        }
                    }
                    
                    // Lire filter_intensity (1-10, défaut: 5)
                    uint8_t filter_intensity = JSONParser::extractInt(pinConfig, "filterIntensity", 5);
                    if (filter_intensity < 1) filter_intensity = 1;
                    if (filter_intensity > 10) filter_intensity = 10;
                    config->filter_intensity = filter_intensity;
                    
                    Serial.printf("[ConfigLoader] Final OSC config: %s addr:%s for GPIO%d\n", 
                                 oscEnabled ? "enabled" : "disabled", config->osc_address, gpio);
                }
            }
        }
        // Serial.printf("[ConfigLoader] Added component: %s on GPIO%d -> %s\n", 
        //              pinLabel.c_str(), gpio, success ? "OK" : "FAILED");
    }
    
    preferences.end();
    // Serial.printf("[ConfigLoader] Loaded %d components from NVS\n", manager.getComponentCount());
}
