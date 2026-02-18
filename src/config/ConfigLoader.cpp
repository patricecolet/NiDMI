#include "ConfigLoader.h"
#include "../managers/ComponentManager.h"
#include "../components/ComponentTypes.h"  // Définitions communes
#include "../components/ComponentRegistry.h"  // Pour trouver les définitions de composants
#include "../components/basic/ButtonDef.h"
#include "../components/basic/LedDef.h"
#include "../components/basic/PotentiometerDef.h"
#include "../components/basic/VelostatDef.h"
#include "../components/basic/JoystickDef.h"
#include "../utils/JSONParser.h"
#include "../utils/PinMapper.h"
#include "../utils/ComponentInitializer.h"  // Pour setupGpio
#include "../managers/complex/ComplexHandlerRegistry.h"
#include "../managers/complex/joystick/JoystickHandler.h"
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
        
        // Joystick : enregistrer le GPIO Y dans le handler (chargement NVS)
        if (type == ComponentType::JOYSTICK) {
            int joyYPin = JSONParser::extractInt(pinConfig, "joyYPin", 255);
            if (joyYPin < 255) {
                ComplexHandler* handler = ComplexHandlerRegistry::getHandler("joystick");
                if (handler) {
                    JoystickHandler* jh = static_cast<JoystickHandler*>(handler);
                    jh->registerYAxis(gpio, (uint8_t)joyYPin);
                }
            }
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
                    
                    // Charger les configurations spécifiques selon le type de composant
                    // La config spécifique doit déjà être allouée par ComponentInitializer
                    switch (config->type) {
                        case ComponentType::BUTTON: {
                            if (config->specificConfig.button) {
                                Components::ButtonConfig* btnConfig = config->specificConfig.button;
                                if (def && def->formFields) {
                                    for (uint8_t i = 0; i < def->formFieldCount && i < MAX_FORM_FIELDS; i++) {
                                        const FormFieldDef& field = def->formFields[i];
                                        if (field.id) {
                                            if (strcmp(field.id, "btnMode") == 0) {
                                                String fieldValue = JSONParser::extractStr(pinConfig, field.id, field.defaultValue ? field.defaultValue : "");
                                                if (fieldValue.length() > 0) {
                                                    strncpy(btnConfig->btnMode, fieldValue.c_str(), sizeof(btnConfig->btnMode) - 1);
                                                    btnConfig->btnMode[sizeof(btnConfig->btnMode) - 1] = '\0';
                                                }
                                            } else if (strcmp(field.id, "btnPulseTiming") == 0) {
                                                String fieldValue = JSONParser::extractStr(pinConfig, field.id, field.defaultValue ? field.defaultValue : "");
                                                if (fieldValue.length() > 0) {
                                                    strncpy(btnConfig->btnPulseTiming, fieldValue.c_str(), sizeof(btnConfig->btnPulseTiming) - 1);
                                                    btnConfig->btnPulseTiming[sizeof(btnConfig->btnPulseTiming) - 1] = '\0';
                                                }
                                            } else if (strcmp(field.id, "btnPullMode") == 0) {
                                                String fieldValue = JSONParser::extractStr(pinConfig, field.id, field.defaultValue ? field.defaultValue : "");
                                                if (fieldValue.length() > 0) {
                                                    strncpy(btnConfig->btnPullMode, fieldValue.c_str(), sizeof(btnConfig->btnPullMode) - 1);
                                                    btnConfig->btnPullMode[sizeof(btnConfig->btnPullMode) - 1] = '\0';
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            break;
                        }
                        case ComponentType::LED: {
                            if (config->specificConfig.led) {
                                Components::LedConfig* ledConfig = config->specificConfig.led;
                                if (def && def->formFields) {
                                    for (uint8_t i = 0; i < def->formFieldCount && i < MAX_FORM_FIELDS; i++) {
                                        const FormFieldDef& field = def->formFields[i];
                                        if (field.id && strcmp(field.id, "ledMode") == 0) {
                                            String fieldValue = JSONParser::extractStr(pinConfig, field.id, field.defaultValue ? field.defaultValue : "");
                                            if (fieldValue.length() > 0) {
                                                strncpy(ledConfig->ledMode, fieldValue.c_str(), sizeof(ledConfig->ledMode) - 1);
                                                ledConfig->ledMode[sizeof(ledConfig->ledMode) - 1] = '\0';
                                            }
                                        }
                                    }
                                }
                            }
                            break;
                        }
                        case ComponentType::POTENTIOMETER: {
                            if (config->specificConfig.potentiometer) {
                                Components::PotentiometerConfig* potConfig = config->specificConfig.potentiometer;
                                int potMin = JSONParser::extractInt(pinConfig, "potMin", 0);
                                int potMax = JSONParser::extractInt(pinConfig, "potMax", 4095);
                                potConfig->potMin = (potMin >= 0 && potMin <= 4095) ? potMin : 0;
                                potConfig->potMax = (potMax >= 0 && potMax <= 4095) ? potMax : 4095;
                                if (potConfig->potMin >= potConfig->potMax) {
                                    uint16_t temp = potConfig->potMin;
                                    potConfig->potMin = potConfig->potMax;
                                    potConfig->potMax = temp;
                                }
                                if (def && def->formFields) {
                                    for (uint8_t i = 0; i < def->formFieldCount && i < MAX_FORM_FIELDS; i++) {
                                        const FormFieldDef& field = def->formFields[i];
                                        if (field.id && strcmp(field.id, "filterIntensity") == 0) {
                                            uint8_t filter_intensity = JSONParser::extractInt(pinConfig, "filterIntensity", field.defaultValue ? atoi(field.defaultValue) : 5);
                                            if (filter_intensity < 1) filter_intensity = 1;
                                            if (filter_intensity > 10) filter_intensity = 10;
                                            potConfig->filter_intensity = filter_intensity;
                                        }
                                    }
                                }
                            }
                            break;
                        }
                        case ComponentType::VELOSTAT: {
                            if (config->specificConfig.velostat) {
                                Components::VelostatConfig* veloConfig = config->specificConfig.velostat;
                                if (def && def->formFields) {
                                    for (uint8_t i = 0; i < def->formFieldCount && i < MAX_FORM_FIELDS; i++) {
                                        const FormFieldDef& field = def->formFields[i];
                                        if (field.id) {
                                            if (strcmp(field.id, "filterIntensity") == 0) {
                                                uint8_t filter_intensity = JSONParser::extractInt(pinConfig, "filterIntensity", field.defaultValue ? atoi(field.defaultValue) : 5);
                                                if (filter_intensity < 1) filter_intensity = 1;
                                                if (filter_intensity > 10) filter_intensity = 10;
                                                veloConfig->filter_intensity = filter_intensity;
                                            } else if (strcmp(field.id, "velocityThreshold") == 0) {
                                                veloConfig->velocityThreshold = JSONParser::extractInt(pinConfig, "velocityThreshold", field.defaultValue ? atoi(field.defaultValue) : 50);
                                            } else if (strcmp(field.id, "aftertouchThreshold") == 0) {
                                                veloConfig->aftertouchThreshold = JSONParser::extractInt(pinConfig, "aftertouchThreshold", field.defaultValue ? atoi(field.defaultValue) : 4);
                                            }
                                        }
                                    }
                                }
                            }
                            break;
                        }
                        case ComponentType::JOYSTICK: {
                            if (config->specificConfig.joystick) {
                                Components::JoystickConfig* joyConfig = config->specificConfig.joystick;
                                joyConfig->joyXMin = JSONParser::extractInt(pinConfig, "xMin", 200);
                                joyConfig->joyXZeroMin = JSONParser::extractInt(pinConfig, "xZeroMin", 1900);
                                joyConfig->joyXZeroMax = JSONParser::extractInt(pinConfig, "xZeroMax", 2100);
                                joyConfig->joyXMax = JSONParser::extractInt(pinConfig, "xMax", 4000);
                                joyConfig->joyYMin = JSONParser::extractInt(pinConfig, "yMin", 200);
                                joyConfig->joyYZeroMin = JSONParser::extractInt(pinConfig, "yZeroMin", 1900);
                                joyConfig->joyYZeroMax = JSONParser::extractInt(pinConfig, "yZeroMax", 2100);
                                joyConfig->joyYMax = JSONParser::extractInt(pinConfig, "yMax", 4000);
                                // Valider les valeurs (0-4095)
                                if (joyConfig->joyXMin > 4095) joyConfig->joyXMin = 200;
                                if (joyConfig->joyXZeroMin > 4095) joyConfig->joyXZeroMin = 1900;
                                if (joyConfig->joyXZeroMax > 4095) joyConfig->joyXZeroMax = 2100;
                                if (joyConfig->joyXMax > 4095) joyConfig->joyXMax = 4000;
                                if (joyConfig->joyYMin > 4095) joyConfig->joyYMin = 200;
                                if (joyConfig->joyYZeroMin > 4095) joyConfig->joyYZeroMin = 1900;
                                if (joyConfig->joyYZeroMax > 4095) joyConfig->joyYZeroMax = 2100;
                                if (joyConfig->joyYMax > 4095) joyConfig->joyYMax = 4000;
                                if (def && def->formFields) {
                                    for (uint8_t i = 0; i < def->formFieldCount && i < MAX_FORM_FIELDS; i++) {
                                        const FormFieldDef& field = def->formFields[i];
                                        if (field.id && strcmp(field.id, "filterIntensity") == 0) {
                                            uint8_t filter_intensity = JSONParser::extractInt(pinConfig, "filterIntensity", field.defaultValue ? atoi(field.defaultValue) : 5);
                                            if (filter_intensity < 1) filter_intensity = 1;
                                            if (filter_intensity > 10) filter_intensity = 10;
                                            joyConfig->filter_intensity = filter_intensity;
                                        }
                                    }
                                }
                                
                                // Charger les types MIDI par axe
                                String xMsgTypeStr = JSONParser::extractStr(pinConfig, "midiMessageTypeX", "");
                                String yMsgTypeStr = JSONParser::extractStr(pinConfig, "midiMessageTypeY", "");
                                if (xMsgTypeStr.length() > 0) {
                                    joyConfig->xMsgType = stringToMidiMessageType(xMsgTypeStr);
                                } else {
                                    // Fallback: utiliser le msg_type global (déjà parsé)
                                    joyConfig->xMsgType = msg_type;
                                }
                                if (yMsgTypeStr.length() > 0) {
                                    joyConfig->yMsgType = stringToMidiMessageType(yMsgTypeStr);
                                } else {
                                    joyConfig->yMsgType = joyConfig->xMsgType; // Même type que X par défaut
                                }
                                
                                // Charger les paramètres MIDI par axe (préfixés X_/Y_)
                                // Le param dépend du type de message sélectionné
                                // CC -> midiCc, Note/Sweep -> midiNote, PitchBend/Aftertouch -> pas de param
                                auto loadAxisParam = [&](const char* axisPrefix, MidiMessageType type, uint8_t fallback) -> uint8_t {
                                    char key[20];
                                    if (type == MidiMessageType::CONTROL_CHANGE) {
                                        snprintf(key, sizeof(key), "%s_midiCc", axisPrefix);
                                        return JSONParser::extractInt(pinConfig, key, 
                                            JSONParser::extractInt(pinConfig, "midiCc", fallback));
                                    } else if (type == MidiMessageType::NOTE_SWEEP || type == MidiMessageType::NOTE) {
                                        snprintf(key, sizeof(key), "%s_midiNote", axisPrefix);
                                        return JSONParser::extractInt(pinConfig, key, 
                                            JSONParser::extractInt(pinConfig, "midiNote", fallback));
                                    }
                                    return fallback;
                                };
                                joyConfig->xMidiParam = loadAxisParam("X", joyConfig->xMsgType, midi_param);
                                joyConfig->yMidiParam = loadAxisParam("Y", joyConfig->yMsgType, midi_param);
                                joyConfig->xMidiChannel = JSONParser::extractInt(pinConfig, "X_midiChannel",
                                    JSONParser::extractInt(pinConfig, "midiChannel", channel));
                                joyConfig->yMidiChannel = JSONParser::extractInt(pinConfig, "Y_midiChannel",
                                    JSONParser::extractInt(pinConfig, "midiChannel", channel));
                                    
                                // Aussi mettre à jour le msg_type principal pour le X
                                config->msg_type = joyConfig->xMsgType;
                                
                                Serial.printf("[ConfigLoader] Joystick MIDI: X=%d(ch%d,p%d) Y=%d(ch%d,p%d)\n",
                                    (int)joyConfig->xMsgType, joyConfig->xMidiChannel, joyConfig->xMidiParam,
                                    (int)joyConfig->yMsgType, joyConfig->yMidiChannel, joyConfig->yMidiParam);
                            }
                            break;
                        }
                        default:
                            // Pas de config spécifique pour ce type
                            break;
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
                    
                    // Note: filter_intensity est maintenant chargé dans les configs spécifiques ci-dessus
                    
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
