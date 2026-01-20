#include "ConfigLoader.h"
#include "../managers/ComponentManager.h"
#include "../components/ComponentTypes.h"  // Définitions communes
#include "../components/ComponentRegistry.h"  // Pour trouver les définitions de composants
#include "../utils/JSONParser.h"
#include "../utils/PinMapper.h"
#include "../utils/ComponentInitializer.h"  // Pour setupGpio
#include "../midi/MidiMessageType.h"
#include <Preferences.h>

void ConfigLoader::loadFromNVS(ComponentManager& manager) {
    Preferences preferences;
    preferences.begin("nidmi", true);
    
    // Serial.println("[ConfigLoader] Loading configs from NVS...");
    
    // Charger les configurations depuis NVS
    // Les clés sont sauvegardées comme "pin_A0", "pin_D2", etc.
    // Générer les labels dynamiquement pour éviter le dépassement de pile
    // au lieu d'un tableau String[200] sur la pile (~6400 bytes)
    
    for (int i = 0; i < 200; i++) { // Max 200 pins possibles (A0-A99 + D0-D99)
        // Générer le label dynamiquement : A0-A99, puis D0-D99
        // Réduire la portée des String pour éviter le dépassement de pile
        const char* pinLabelCStr;
        char pinLabelBuf[8]; // Suffisant pour "A99" ou "D99"
        if (i < 100) {
            snprintf(pinLabelBuf, sizeof(pinLabelBuf), "A%d", i);
        } else {
            snprintf(pinLabelBuf, sizeof(pinLabelBuf), "D%d", i - 100);
        }
        pinLabelCStr = pinLabelBuf;
        
        // Construire la clé sans String intermédiaire
        char keyBuf[16]; // "pin_A99" ou "pin_D99"
        snprintf(keyBuf, sizeof(keyBuf), "pin_%s", pinLabelCStr);
        
        if (!preferences.isKey(keyBuf)) {
            continue; // Passer au suivant
        }
        
        // Lire la config - limiter strictement la portée de la String
        String pinConfig = preferences.getString(keyBuf, "");
        if (pinConfig.length() == 0) {
            continue;
        }
        
        // Extraire role - garder en vie jusqu'à l'utilisation
        String role = JSONParser::extractStr(pinConfig, "role", "\n");
        if (role.length() == 0) continue;
        
        // Trouver la définition du composant via ComponentRegistry
        const ComponentDefinition* def = ComponentRegistry::findById(role.c_str());
        if (!def) {
            Serial.printf("[ConfigLoader] WARNING: Component '%s' not found in registry for pin %s\n", 
                          role.c_str(), pinLabelCStr);
            continue;
        }
        
        // Utiliser PinMapper pour obtenir le GPIO (accepte const char*)
        uint8_t gpio = PinMapper::labelToGpio(pinLabelCStr);
        if (gpio == 255) {
            Serial.printf("[ConfigLoader] Invalid pin label: %s (GPIO=255)\n", pinLabelCStr);
            continue;
        }
        
        // Vérifier que la pin a les capacités requises selon pinType
        if (def->pinType == PinType::PIN_ANALOG) {
            if (!PinMapper::hasAdc(gpio)) {
                Serial.printf("[ConfigLoader] WARNING: Pin %s (GPIO%d) n'a pas d'ADC, ignorée\n", 
                              pinLabelCStr, gpio);
                continue;
            }
        }
        
        // Extraire paramètres MIDI
        uint8_t midi_param = 7; // défaut CC
        uint8_t channel = 1;    // défaut canal 1
        MidiMessageType msg_type = MidiMessageType::NOTE; // défaut
        
        // Lire midiMessageType (ou rtpType pour compatibilité) - garder en vie jusqu'à l'utilisation
        String rtpTypeStr = JSONParser::extractStr(pinConfig, "midiMessageType", "");
        if (rtpTypeStr.length() == 0) {
            rtpTypeStr = JSONParser::extractStr(pinConfig, "rtpType", ""); // Compatibilité ancien format
        }
        
        // Chercher le MidiMessageDef correspondant
        const MidiMessageDef* msgDef = nullptr;
        if (rtpTypeStr.length() > 0) {
            // Chercher par displayName d'abord, puis par id
            for (uint8_t i = 0; i < def->midiMessageCount && i < MAX_MIDI_MESSAGES; i++) {
                if (def->midiMessages[i].displayName && strcmp(def->midiMessages[i].displayName, rtpTypeStr.c_str()) == 0) {
                    msgDef = &def->midiMessages[i];
                    break;
                }
            }
            // Si pas trouvé par displayName, chercher par id
            if (!msgDef) {
                for (uint8_t i = 0; i < def->midiMessageCount && i < MAX_MIDI_MESSAGES; i++) {
                    if (def->midiMessages[i].id && strcmp(def->midiMessages[i].id, rtpTypeStr.c_str()) == 0) {
                        msgDef = &def->midiMessages[i];
                        break;
                    }
                }
            }
            // Convertir en MidiMessageType pour msg_type
            if (msgDef && msgDef->displayName) {
                msg_type = stringToMidiMessageType(msgDef->displayName);
            } else {
                msg_type = stringToMidiMessageType(rtpTypeStr);
            }
        } else {
            // Défaut : utiliser le premier message MIDI du composant s'il existe
            if (def->midiMessageCount > 0) {
                msgDef = &def->midiMessages[0];
                if (msgDef->id) {
                    msg_type = stringToMidiMessageType(msgDef->id);
                }
            }
        }
        
        // Extraire les paramètres MIDI selon les paramètres définis dans MidiMessageDef
        if (msgDef) {
            // Lire midiChannel (ou rtpChan pour compatibilité)
            channel = JSONParser::extractInt(pinConfig, "midiChannel", 1);
            if (channel == 1) {
                channel = JSONParser::extractInt(pinConfig, "rtpChan", 1); // Compatibilité ancien format
            }
            
            // Parcourir les paramètres du message pour extraire la valeur appropriée
            for (uint8_t i = 0; i < msgDef->paramCount && i < MAX_MIDI_PARAMS; i++) {
                const MidiParamDef& param = msgDef->params[i];
                if (param.id) {
                    // Extraire selon le type de paramètre (nouveau format puis ancien pour compatibilité)
                    if (strcmp(param.id, "midiNote") == 0) {
                        midi_param = JSONParser::extractInt(pinConfig, "midiNote", param.defaultValue ? atoi(param.defaultValue) : 60);
                        if (midi_param == (param.defaultValue ? atoi(param.defaultValue) : 60)) {
                            midi_param = JSONParser::extractInt(pinConfig, "rtpNote", midi_param); // Compatibilité
                        }
                    } else if (strcmp(param.id, "midiCc") == 0) {
                        midi_param = JSONParser::extractInt(pinConfig, "midiCc", param.defaultValue ? atoi(param.defaultValue) : 7);
                        if (midi_param == (param.defaultValue ? atoi(param.defaultValue) : 7)) {
                            midi_param = JSONParser::extractInt(pinConfig, "rtpCc", midi_param); // Compatibilité
                        }
                    } else if (strcmp(param.id, "midiPc") == 0) {
                        // Lire midiPc avec défaut 1 (1-based, comme le formulaire)
                        midi_param = JSONParser::extractInt(pinConfig, "midiPc", 1);
                        if (midi_param == 1) {
                            // Compatibilité : essayer ancien format (0-based)
                            int old_value = JSONParser::extractInt(pinConfig, "rtpPc", -1);
                            if (old_value >= 0 && old_value <= 127) {
                                // Ancienne valeur 0-based trouvée : convertir en 1-based pour cohérence
                                midi_param = old_value + 1;
                            }
                        } else if (midi_param == 0) {
                            // Valeur 0 trouvée : probablement ancienne valeur 0-based, convertir en 1-based
                            // Chercher rtpPc pour confirmation
                            int old_value = JSONParser::extractInt(pinConfig, "rtpPc", -1);
                            if (old_value >= 0 && old_value <= 127) {
                                midi_param = old_value + 1; // Convertir 0-based → 1-based
                            } else {
                                midi_param = 1; // Par défaut : programme 1
                            }
                        }
                        // S'assurer que le programme est dans la plage valide (1-128)
                        if (midi_param < 1) midi_param = 1;
                        if (midi_param > 128) midi_param = 128;
                    } else if (strcmp(param.id, "midiCcRange") == 0 && param.type == FieldType::RANGE) {
                        // Plage MIDI : lire midiCcRangeMin et midiCcRangeMin
                        // Les valeurs seront stockées après l'ajout du composant
                    }
                    // Les autres paramètres (midiChannel, midiVelocity, etc.) sont lus ailleurs
                }
            }
        } else {
            // Fallback si pas de définition de message
            channel = JSONParser::extractInt(pinConfig, "midiChannel", 1);
            if (channel == 1) {
                channel = JSONParser::extractInt(pinConfig, "rtpChan", 1); // Compatibilité
            }
            if (msg_type == MidiMessageType::CONTROL_CHANGE) {
                midi_param = JSONParser::extractInt(pinConfig, "midiCc", 7);
                if (midi_param == 7) {
                    midi_param = JSONParser::extractInt(pinConfig, "rtpCc", 7); // Compatibilité
                }
            } else if (msg_type == MidiMessageType::PROGRAM_CHANGE) {
                // Lire midiPc avec défaut 1 (1-based, comme le formulaire)
                midi_param = JSONParser::extractInt(pinConfig, "midiPc", 1);
                if (midi_param == 1) {
                    // Compatibilité : essayer ancien format (0-based)
                    int old_value = JSONParser::extractInt(pinConfig, "rtpPc", -1);
                    if (old_value >= 0 && old_value <= 127) {
                        // Ancienne valeur 0-based trouvée : convertir en 1-based pour cohérence
                        midi_param = old_value + 1;
                    }
                }
                // S'assurer que le programme est dans la plage valide (1-128)
                if (midi_param < 1) midi_param = 1;
                if (midi_param > 128) midi_param = 128;
            } else if (msg_type == MidiMessageType::NOTE || msg_type == MidiMessageType::NOTE_VELOCITY || msg_type == MidiMessageType::NOTE_SWEEP) {
                midi_param = JSONParser::extractInt(pinConfig, "midiNote", 60);
                if (midi_param == 60) {
                    midi_param = JSONParser::extractInt(pinConfig, "rtpNote", 60); // Compatibilité
                }
            }
        }
        
        // Ajouter le composant en utilisant le type depuis la définition
        ComponentType type = def->type;
        
        // DEBUG: Afficher les valeurs extraites
        Serial.printf("[ConfigLoader] Pin %s: type=%d, midi_param=%d, channel=%d, msg_type=%d\n", 
                     pinLabelCStr, (int)type, midi_param, channel, (int)msg_type);
        
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
                    
                    // Configurer les flags (bit 0x02 pour OSC, bit 0x04 pour format MIDI, bit 0x08 pour format RAW)
                    if (oscEnabled) {
                        config->flags |= 0x02; // Activer OSC
                        if (oscFormat.length() > 0 && oscFormat.equalsIgnoreCase("midi")) {
                            config->flags |= 0x04; // Format MIDI
                            config->flags &= ~0x08; // Pas RAW
                        } else if (oscFormat.length() > 0 && oscFormat.equalsIgnoreCase("raw")) {
                            config->flags |= 0x08; // Format RAW
                            config->flags &= ~0x04; // Pas MIDI
                        } else {
                            config->flags &= ~0x04; // Format float
                            config->flags &= ~0x08; // Pas RAW
                        }
                    } else {
                        config->flags &= ~0x02; // Désactiver OSC
                        config->flags &= ~0x04; // Pas MIDI
                        config->flags &= ~0x08; // Pas RAW
                    }
                    
                    // Configurer l'adresse OSC (utiliser valeur par défaut si vide)
                    if (oscAddress.length() > 0) {
                        strncpy(config->osc_address, oscAddress.c_str(), sizeof(config->osc_address) - 1);
                        config->osc_address[sizeof(config->osc_address) - 1] = '\0';
                        Serial.printf("[ConfigLoader] OSC address from config: '%s' for %s\n", 
                                      oscAddress.c_str(), pinLabelCStr);
                    } else {
                        Serial.printf("[ConfigLoader] OSC address empty for %s, using default: '%s'\n", 
                                      pinLabelCStr, config->osc_address);
                    }
                    
                    // Lire dynamiquement tous les formFields depuis la définition
                    if (def && def->formFields) {
                        uint8_t customFieldIndex = 0;  // Index pour mapper vers customField1/customField2
                        uint8_t customIntIndex = 0;   // Index pour mapper vers customInt1/customInt2
                        
                        for (uint8_t i = 0; i < def->formFieldCount && i < MAX_FORM_FIELDS; i++) {
                            const FormFieldDef& field = def->formFields[i];
                            if (field.id) {
                                // Mapper les champs spécifiques existants (réduire la portée des String)
                                if (strcmp(field.id, "btnMode") == 0) {
                                    {
                                        String fieldValue = JSONParser::extractStr(pinConfig, field.id, field.defaultValue ? field.defaultValue : "");
                                        if (fieldValue.length() > 0) {
                                            strncpy(config->btnMode, fieldValue.c_str(), sizeof(config->btnMode) - 1);
                                            config->btnMode[sizeof(config->btnMode) - 1] = '\0';
                                        }
                                    }
                                } else if (strcmp(field.id, "btnPulseTiming") == 0) {
                                    {
                                        String fieldValue = JSONParser::extractStr(pinConfig, field.id, field.defaultValue ? field.defaultValue : "");
                                        if (fieldValue.length() > 0) {
                                            strncpy(config->btnPulseTiming, fieldValue.c_str(), sizeof(config->btnPulseTiming) - 1);
                                            config->btnPulseTiming[sizeof(config->btnPulseTiming) - 1] = '\0';
                                        }
                                    }
                                } else if (strcmp(field.id, "btnPullMode") == 0) {
                                    {
                                        String fieldValue = JSONParser::extractStr(pinConfig, field.id, field.defaultValue ? field.defaultValue : "");
                                        if (fieldValue.length() > 0) {
                                            strncpy(config->btnPullMode, fieldValue.c_str(), sizeof(config->btnPullMode) - 1);
                                            config->btnPullMode[sizeof(config->btnPullMode) - 1] = '\0';
                                        }
                                    }
                                } else if (strcmp(field.id, "ledMode") == 0) {
                                    {
                                        String fieldValue = JSONParser::extractStr(pinConfig, field.id, field.defaultValue ? field.defaultValue : "");
                                        if (fieldValue.length() > 0) {
                                            strncpy(config->ledMode, fieldValue.c_str(), sizeof(config->ledMode) - 1);
                                            config->ledMode[sizeof(config->ledMode) - 1] = '\0';
                                        }
                                    }
                                } else if (strcmp(field.id, "filterIntensity") == 0) {
                                    uint8_t filter_intensity = JSONParser::extractInt(pinConfig, "filterIntensity", field.defaultValue ? atoi(field.defaultValue) : 5);
                                    if (filter_intensity < 1) filter_intensity = 1;
                                    if (filter_intensity > 10) filter_intensity = 10;
                                    config->filter_intensity = filter_intensity;
                                } else {
                                    // Mapper vers les champs génériques pour les nouveaux composants
                                    if (field.type == FieldType::TEXT || field.type == FieldType::SELECT || field.type == FieldType::CHECKBOX) {
                                        {
                                            String fieldValue = JSONParser::extractStr(pinConfig, field.id, field.defaultValue ? field.defaultValue : "");
                                            if (fieldValue.length() > 0) {
                                                const char* fieldValueCStr = fieldValue.c_str();
                                                if (customFieldIndex == 0) {
                                                    strncpy(config->customField1, fieldValueCStr, sizeof(config->customField1) - 1);
                                                    config->customField1[sizeof(config->customField1) - 1] = '\0';
                                                    customFieldIndex++;
                                                } else if (customFieldIndex == 1) {
                                                    strncpy(config->customField2, fieldValueCStr, sizeof(config->customField2) - 1);
                                                    config->customField2[sizeof(config->customField2) - 1] = '\0';
                                                    customFieldIndex++;
                                                }
                                            }
                                        }
                                    } else if (field.type == FieldType::NUMBER || field.type == FieldType::RANGE) {
                                        int fieldValue = JSONParser::extractInt(pinConfig, field.id, field.defaultValue ? atoi(field.defaultValue) : 0);
                                        if (customIntIndex == 0) {
                                            config->customInt1 = fieldValue;
                                            customIntIndex++;
                                        } else if (customIntIndex == 1) {
                                            config->customInt2 = fieldValue;
                                            customIntIndex++;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    
                    // Lire potMin et potMax pour potentiomètre (seuils analogiques)
                    if (strcmp(def->id, "potentiometer") == 0) {
                        int potMin = JSONParser::extractInt(pinConfig, "potMin", 0);
                        int potMax = JSONParser::extractInt(pinConfig, "potMax", 4095);
                        // Stocker dans potMin et potMax
                        config->potMin = (potMin >= 0 && potMin <= 4095) ? potMin : 0;
                        config->potMax = (potMax >= 0 && potMax <= 4095) ? potMax : 4095;
                        // S'assurer que min < max
                        if (config->potMin >= config->potMax) {
                            uint16_t temp = config->potMin;
                            config->potMin = config->potMax;
                            config->potMax = temp;
                        }
                    }
                    
                    // Lire midiCcRangeMin et midiCcRangeMax (plage MIDI)
                    int midiCcRangeMin = JSONParser::extractInt(pinConfig, "midiCcRangeMin", 0);
                    int midiCcRangeMax = JSONParser::extractInt(pinConfig, "midiCcRangeMax", 127);
                    // Valider et stocker
                    config->midiCcRangeMin = (midiCcRangeMin >= 0 && midiCcRangeMin <= 127) ? midiCcRangeMin : 0;
                    config->midiCcRangeMax = (midiCcRangeMax >= 0 && midiCcRangeMax <= 127) ? midiCcRangeMax : 127;
                    // S'assurer que min < max
                    if (config->midiCcRangeMin >= config->midiCcRangeMax) {
                        uint8_t temp = config->midiCcRangeMin;
                        config->midiCcRangeMin = config->midiCcRangeMax;
                        config->midiCcRangeMax = temp;
                    }
                    
                    // Lire midiCcOnOffMin et midiCcOnOffMax pour boutons en CC
                    int midiCcOnOffMin = JSONParser::extractInt(pinConfig, "midiCcOnOffMin", 0);
                    int midiCcOnOffMax = JSONParser::extractInt(pinConfig, "midiCcOnOffMax", 127);
                    // Valider et stocker
                    config->midiCcOnOffMin = (midiCcOnOffMin >= 0 && midiCcOnOffMin <= 127) ? midiCcOnOffMin : 0;
                    config->midiCcOnOffMax = (midiCcOnOffMax >= 0 && midiCcOnOffMax <= 127) ? midiCcOnOffMax : 127;
                    
                    // Lire les paramètres pour NOTE_SWEEP (balayage)
                    if (msg_type == MidiMessageType::NOTE_SWEEP) {
                        // Lire avec nouveaux noms puis anciens pour compatibilité
                        config->rtpNoteMin = JSONParser::extractInt(pinConfig, "midiNoteMin", 48);
                        if (config->rtpNoteMin == 48) {
                            config->rtpNoteMin = JSONParser::extractInt(pinConfig, "rtpNoteMin", 48); // Compatibilité
                        }
                        config->rtpNoteMax = JSONParser::extractInt(pinConfig, "midiNoteMax", 72);
                        if (config->rtpNoteMax == 72) {
                            config->rtpNoteMax = JSONParser::extractInt(pinConfig, "rtpNoteMax", 72); // Compatibilité
                        }
                        config->rtpNoteVelFix = JSONParser::extractInt(pinConfig, "midiNoteVelocityFix", 100);
                        if (config->rtpNoteVelFix == 100) {
                            config->rtpNoteVelFix = JSONParser::extractInt(pinConfig, "rtpNoteVelFix", 100); // Compatibilité
                        }
                        config->rtpNoteSweepAutoOffDelay = JSONParser::extractInt(pinConfig, "midiNoteSweepAutoOffDelay", 0);
                        if (config->rtpNoteSweepAutoOffDelay == 0) {
                            config->rtpNoteSweepAutoOffDelay = JSONParser::extractInt(pinConfig, "rtpNoteSweepAutoOffDelay", 0); // Compatibilité
                        }
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
                    
                    // Reconfigurer le GPIO après avoir chargé tous les champs (notamment btnPullMode pour les boutons)
                    ComponentInitializer::setupGpio(gpio, type, config);
                    
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
