#include "APICommon.h"
#include "../utils/PinMapper.h"
#include "../managers/ComponentManager.h"
#include "../server/ServerCallbacks.h" /* Pour nidmi_requestReloadPins */
#include "../hardware/MuxConstants.h"
#include "../Globals.h"
#include "../config/ConfigCache.h"
#include "../components/ComponentRegistry.h"
#include "../components/ValidationRegistry.h"
#include "../managers/complex/ComplexHandlerRegistry.h"

/* Forward declarations */
String getDefaultConfig(String pin);

/* Limite NVS : une valeur ne doit pas dépasser ~1984 octets (ESP-IDF). Garder marge pour éviter corruption quand on sauvegarde beaucoup de pins. */
static const size_t NVS_MAX_PIN_CONFIG_SIZE = 1900U;

void setupPinAPI(AsyncWebServer& server) {
    /* API - Capacités des pins (dynamique selon MCU) */
    server.on("/api/pins/caps", HTTP_GET, [](AsyncWebServerRequest *request){
        /* Détecter le MCU */
        PinMapper::detectMcu();
        
        /* Construire JSON dynamique */
        String json = "{";
        String mcuName = PinMapper::getMcuName();
        mcuName.toLowerCase();
        json += "\"board\":\"" + mcuName + "\",";
        json += "\"pins\":[";
        
        const PinMapping* mappings = PinMapper::getAllMappings();
        size_t count = PinMapper::getMappingCount();
        
        for (size_t i = 0; i < count; i++) {
            if (i > 0) json += ",";
            json += "{";
            json += "\"gpio\":" + String(mappings[i].gpio) + ",";
            json += "\"label\":\"" + String(mappings[i].label) + "\",";
            json += "\"caps\":{";
            json += "\"in\":true,";
            json += "\"out\":true,";
            json += "\"adc\":" + String(mappings[i].has_adc ? "true" : "false") + ",";
            json += "\"pwm\":" + String(mappings[i].has_pwm ? "true" : "false") + ",";
            json += "\"touch\":" + String(mappings[i].has_touch ? "true" : "false");
            json += "},";
            json += "\"sensitive\":false";
            json += "}";
        }
        
        json += "],";
        json += "\"bus\":{";
        
        /* Bus I2C - Utiliser PinMapper pour obtenir les GPIO dynamiquement */
        json += "\"i2c\":{";
        uint8_t sda_gpio = PinMapper::labelToGpio("SDA");
        uint8_t scl_gpio = PinMapper::labelToGpio("SCL");
        json += "\"sda\":" + String(sda_gpio) + ",\"scl\":" + String(scl_gpio);
        json += "},";
        
        /* Bus SPI - Utiliser PinMapper pour obtenir les GPIO dynamiquement */
        json += "\"spi\":{";
        uint8_t mosi_gpio = PinMapper::labelToGpio("MOSI");
        uint8_t miso_gpio = PinMapper::labelToGpio("MISO");
        uint8_t sck_gpio = PinMapper::labelToGpio("SCK");
        json += "\"mosi\":" + String(mosi_gpio) + ",\"miso\":" + String(miso_gpio) + ",\"sck\":" + String(sck_gpio);
        json += "},";
        
        /* Bus UART - Utiliser PinMapper pour obtenir les GPIO dynamiquement */
        json += "\"uart\":{";
        uint8_t tx_gpio = PinMapper::labelToGpio("TX");
        uint8_t rx_gpio = PinMapper::labelToGpio("RX");
        json += "\"tx\":" + String(tx_gpio) + ",\"rx\":" + String(rx_gpio);
        json += "}";
        
        json += "}";
        json += "}";
        
        request->send(200, "application/json", json);
    });

    /* API - Liste des pins configurées (format pour saveAll avec pinLabel) */
    /* Inclut aussi les composants complexes depuis MuxManager */
    server.on("/api/pins/list", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = "{";
        json += "\"pins\":[";
        
        Preferences preferences;
        preferences.begin("nidmi", true);
        
        bool first = true;
        
        /* Obtenir toutes les pins disponibles pour ce MCU */
        PinMapper::detectMcu();
        const PinMapping* mappings = PinMapper::getAllMappings();
        size_t mapping_count = PinMapper::getMappingCount();
        
        /* Charger les pins simples depuis NVS */
        for (size_t i = 0; i < mapping_count; i++) {
            String pinLabel = String(mappings[i].label);
            String key = "pin_" + pinLabel;
            String configStr = preferences.getString(key.c_str(), "");
            if (!configStr.isEmpty()) {
                /* Ignorer les pins qui ont additionalPins (seront ajoutées depuis MuxManager) */
                if(configStr.indexOf("\"additionalPins\"") >= 0) {
                    continue;
                }
                if (!first) json += ",";
                json += configStr;  // Le config est déjà un JSON complet avec pinLabel
                first = false;
            }
        }
        
        /* Ajouter les composants complexes depuis MuxManager */
        for (uint8_t i = 0; i < MAX_MUXES; i++) {
            const MuxConfig* cfg = g_componentManager.getMuxConfig(i);
            if (cfg && cfg->enabled) {
                /* Chercher d'abord le pinLabel depuis NVS (pinLabel original sauvegardé) */
                String sigPinLabel = "";
                /* Essayer d'abord la clé générique, puis la clé spécifique MUX pour compatibilité */
                String keyNVS = "pinLabel_complex_" + String(i);
                String savedPinLabel = preferences.getString(keyNVS.c_str(), "");
                if(savedPinLabel.isEmpty()) {
                    /* Fallback pour compatibilité avec l'ancien format */
                    keyNVS = "pinLabel_mux_" + String(i);
                    savedPinLabel = preferences.getString(keyNVS.c_str(), "");
                }
                if(!savedPinLabel.isEmpty()) {
                    sigPinLabel = savedPinLabel;
                } else {
                    /* Si pas trouvé dans NVS, trouver le pinLabel correspondant au GPIO SIG */
                    /* Préférer les labels analogiques (commencent par "A") si disponibles */
                    String analogPinLabel = "";
                    for (size_t j = 0; j < mapping_count; j++) {
                        if(mappings[j].gpio == cfg->sig_pin) {
                            String label = String(mappings[j].label);
                            if(label.startsWith("A")) {
                                analogPinLabel = label;
                                break;
                            } else if(sigPinLabel.isEmpty()) {
                                sigPinLabel = label;
                            }
                        }
                    }
                    if(!analogPinLabel.isEmpty()) {
                        sigPinLabel = analogPinLabel;
                    }
                }
                
                if(!sigPinLabel.isEmpty()) {
                    /* Essayer d'abord de lire depuis NVS (contient tous les paramètres sauvegardés) */
                    String key = "pin_" + sigPinLabel;
                    String configStr = preferences.getString(key.c_str(), "");
                    
                    if(!configStr.isEmpty()) {
                        /* Si la config existe dans NVS, l'utiliser directement (contient tous les paramètres MIDI, formFields, etc.) */
                        if (!first) json += ",";
                        json += configStr;  /* Le config est déjà un JSON complet avec pinLabel */
                        first = false;
                    } else {
                        /* Fallback : construire depuis MuxConfig si pas trouvé dans NVS (compatibilité) */
                        if (!first) json += ",";
                        
                        /* Obtenir le rôle depuis NVS (stocké lors de la sauvegarde) */
                        String roleKey = "role_complex_" + String(i);
                        String role = preferences.getString(roleKey.c_str(), "");
                        if(role.isEmpty()) {
                            /* Fallback pour compatibilité avec l'ancien format */
                            roleKey = "role_mux_" + String(i);
                            role = preferences.getString(roleKey.c_str(), "");
                        }
                        /* Fallback : si pas trouvé, utiliser "hc4067" par défaut pour compatibilité */
                        if(role.isEmpty()) {
                            role = "hc4067";
                        }
                        
                        /* Construire le JSON unifié avec additionalPins en utilisant le handler générique */
                        ComplexHandler* handler = ComplexHandlerRegistry::getHandler(role.c_str());
                        if(handler) {
                            /* Utiliser le handler générique pour obtenir les infos */
                            json += "{";
                            json += "\"pinLabel\":\"" + sigPinLabel + "\",";
                            json += "\"role\":\"" + role + "\",";
                            String infoJson = "";
                            if(handler->getComponentInfo(sigPinLabel.c_str(), cfg->sig_pin, infoJson)) {
                                json += infoJson;  /* Ajoute additionalPins, formFields, midiParams, etc. */
                            } else {
                                /* Fallback si getComponentInfo échoue */
                                json += "\"additionalPins\":{},";
                                json += "\"min\":0,\"max\":4095,";
                                json += "\"midiCc\":1,\"midiChannel\":1,\"rtpMidiEnabled\":true";
                            }
                            json += "}";
                        } else {
                            /* Fallback si handler non trouvé (ne devrait pas arriver) */
                            json += "{";
                            json += "\"pinLabel\":\"" + sigPinLabel + "\",";
                            json += "\"role\":\"" + role + "\",";
                            json += "\"additionalPins\":{},";
                            json += "\"min\":0,\"max\":4095,";
                            json += "\"rtpCc\":1,\"rtpChan\":1,\"rtpEnabled\":true";
                            json += "}";
                        }
                        first = false;
                    }
                }
            }
        }
        
        preferences.end();
        json += "]}";
        
        request->send(200, "application/json", json);
    });

    /* API - Configuration d'une pin (format paramètres URL pour saveAll) */
    server.on("/api/pins/set", HTTP_POST, [](AsyncWebServerRequest *request){
        if(!request->hasParam("pinLabel", true) || !request->hasParam("role", true)){
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"pinLabel and role required\"}");
            return;
        }
        
        String pinLabel = request->getParam("pinLabel", true)->value();
        String role = request->getParam("role", true)->value();
        
        /* Obtenir la définition du composant pour lecture dynamique des paramètres */
        const ComponentDefinition* def = ComponentRegistry::findById(role.c_str());
        
        /* Trouver le GPIO correspondant au pinLabel */
        uint8_t sigGpio = 255;
        PinMapper::detectMcu();
        const PinMapping* mappings = PinMapper::getAllMappings();
        size_t mapping_count = PinMapper::getMappingCount();
        for (size_t i = 0; i < mapping_count; i++) {
            if(String(mappings[i].label) == pinLabel) {
                sigGpio = mappings[i].gpio;
                break;
            }
        }
        
        /* Construire le JSON à partir des paramètres */
        String json = "{";
        json += "\"pinLabel\":\"" + pinLabel + "\",";
        json += "\"role\":\"" + role + "\"";
        
        auto addParam = [&](const char* name) {
            if(request->hasParam(name, true)) {
                String val = request->getParam(name, true)->value();
                if(val == "true" || val == "false") {
                    json += ",\"" + String(name) + "\":" + val;
                } else if(val.indexOf(',') >= 0) {
                    // Valeurs contenant des virgules (ex. "0,0") → toujours chaînes JSON
                    String escaped = val;
                    escaped.replace("\\", "\\\\");
                    escaped.replace("\"", "\\\"");
                    json += ",\"" + String(name) + "\":\"" + escaped + "\"";
                } else if(val.length() > 0 && ((val[0] >= '0' && val[0] <= '9') ||
                          (val[0] == '-' && val.length() > 1 && (val[1] >= '0' && val[1] <= '9')))) {
                    // Nombres (y compris négatifs) sans virgule
                    json += ",\"" + String(name) + "\":" + val;
                } else {
                    // Chaînes génériques, avec échappement basique
                    String escaped = val;
                    escaped.replace("\\", "\\\\");
                    escaped.replace("\"", "\\\"");
                    json += ",\"" + String(name) + "\":\"" + escaped + "\"";
                }
            }
        };
        
        /* Paramètres MIDI communs (toujours présents) */
        addParam("rtpMidiEnabled"); // Compatibilité: accepter aussi rtpEnabled
        addParam("rtpEnabled"); // Ancien format pour compatibilité
        addParam("midiMessageType"); // Nouveau format
        addParam("rtpType"); // Ancien format pour compatibilité
        
        /* Lire dynamiquement les paramètres MIDI depuis def->midiMessages[].params[] */
        if(def && def->midiMessageCount > 0 && def->midiMessages) {
            for(uint8_t i = 0; i < def->midiMessageCount; i++) {
                const MidiMessageDef& msg = def->midiMessages[i];
                if(msg.params && msg.paramCount > 0) {
                    for(uint8_t j = 0; j < msg.paramCount && j < msg.paramsCapacity; j++) {
                        const MidiParamDef& param = msg.params[j];
                        if(param.id && strlen(param.id) > 0) {
                            /* Traiter les paramètres RANGE comme Min/Max */
                            if(param.type == FieldType::RANGE) {
                                String minId = String(param.id) + "Min";
                                String maxId = String(param.id) + "Max";
                                // Toujours sauvegarder les valeurs (même si par défaut)
                                if(request->hasParam(minId.c_str(), true)) {
                                    addParam(minId.c_str());
                                } else if(param.defaultMin) {
                                    // Sauvegarder la valeur par défaut si absente
                                    json += ",\"" + minId + "\":" + String(param.defaultMin);
                                }
                                if(request->hasParam(maxId.c_str(), true)) {
                                    addParam(maxId.c_str());
                                } else if(param.defaultMax) {
                                    // Sauvegarder la valeur par défaut si absente
                                    json += ",\"" + maxId + "\":" + String(param.defaultMax);
                                }
                            } else {
                                /* Pour autres types (NUMBER, INFO, etc.) */
                                addParam(param.id);
                            }
                        }
                    }
                }
            }
        }
        
        /* Lire dynamiquement les formFields depuis def->formFields[] */
        if(def && def->formFieldCount > 0 && def->formFields) {
            for(uint8_t i = 0; i < def->formFieldCount && i < def->formFieldsCapacity; i++) {
                const FormFieldDef& field = def->formFields[i];
                if(field.id && strlen(field.id) > 0 && field.id[0] != '_') {
                    if(field.type == FieldType::CHECKBOX) {
                        /* Pour checkbox, vérifier que la valeur est "true" */
                        if(request->hasParam(field.id, true)) {
                            String val = request->getParam(field.id, true)->value();
                            if(val == "true") {
                                addParam(field.id);
                            }
                        }
                    } else if(field.type == FieldType::RANGE) {
                        /* Pour RANGE, lire Min et Max */
                        String minId = String(field.id) + "Min";
                        String maxId = String(field.id) + "Max";
                        if(request->hasParam(minId.c_str(), true)) {
                            addParam(minId.c_str());
                        }
                        if(request->hasParam(maxId.c_str(), true)) {
                            addParam(maxId.c_str());
                        }
                    } else {
                        /* Pour autres types (TEXT, NUMBER, SELECT, INFO) */
                        addParam(field.id);
                    }
                }
            }
        }
        
        /* Paramètres communs (OSC, Debug) */
        addParam("oscEnabled");
        addParam("oscAddress");
        addParam("oscFormat");
        addParam("dbgEnabled");
        addParam("dbgHeader");
        
        /* Vérifier si le composant a des additionalPins */
        bool hasAdditionalPins = false;
        
        if(def && def->additionalPinCount > 0 && def->additionalPins) {
            /* Vérifier que tous les paramètres required sont présents */
            hasAdditionalPins = true;
            for(uint8_t i = 0; i < def->additionalPinCount && i < def->additionalPinsCapacity; i++) {
                if(!def->additionalPins[i].optional) {
                    String pinId = String(def->additionalPins[i].id);
                    bool hasParam = request->hasParam(pinId.c_str(), true);
                    if(!hasParam) {
                        hasAdditionalPins = false;
                        break;
                    }
                }
            }
            
            if(hasAdditionalPins) {
                json += ",\"additionalPins\":{";
                bool first = true;
                for(uint8_t i = 0; i < def->additionalPinCount && i < def->additionalPinsCapacity; i++) {
                    const AdditionalPinDef& pin = def->additionalPins[i];
                    if(request->hasParam(pin.id, true)) {
                        if(!first) json += ",";
                        json += "\"" + String(pin.id) + "\":" + request->getParam(pin.id, true)->value();
                        first = false;
                    } else if(!pin.optional) {
                        // Pin requise absente (ne devrait pas arriver après la vérification ci-dessus)
                        hasAdditionalPins = false;
                        break;
                    } else {
                        // Pin optionnelle absente, utiliser la valeur par défaut
                        if(!first) json += ",";
                        json += "\"" + String(pin.id) + "\":" + String(pin.defaultValue);
                        first = false;
                    }
                }
                json += "}";
                
                /* Note: complexId supprimé - plus besoin d'ID explicite */
            }
        }
        
        json += "}";
        
        if (json.length() > NVS_MAX_PIN_CONFIG_SIZE) {
            Serial.printf("[PinAPI] JSON trop gros pour NVS: %u > %u (pin=%s) - refus d'écriture pour éviter corruption\n",
                (unsigned)json.length(), (unsigned)NVS_MAX_PIN_CONFIG_SIZE, pinLabel.c_str());
            request->send(413, "application/json", "{\"status\":\"error\",\"message\":\"Config trop grande pour NVS (max 1900 octets)\"}");
            return;
        }
        
        Preferences preferences;
        preferences.begin("nidmi", false);
        String key = "pin_" + pinLabel;
        size_t written = preferences.putString(key.c_str(), json);
        preferences.end();
        
        if (written == 0) {
            Serial.printf("[PinAPI] ERREUR NVS: putString a échoué pour %s\n", pinLabel.c_str());
        }
        
        /* Si additionalPins présent, utiliser le handler générique pour ce type de composant */
        if(hasAdditionalPins && def) {
            /* Obtenir le handler pour ce type de composant */
            ComplexHandler* handler = ComplexHandlerRegistry::getHandler(role.c_str());
            
            if(handler) {
                /* Construire ComplexComponentData depuis la requête HTTP */
                ComplexComponentData data;
                data.def = def;
                data.pinLabel = pinLabel.c_str();
                data.mainPinGpio = sigGpio;
                
                /* Allouer et remplir additionalPins */
                data.additionalPinCount = def->additionalPinCount;
                data.additionalPins = new ComplexComponentData::AdditionalPinValue[data.additionalPinCount];
                for(uint8_t i = 0; i < def->additionalPinCount && i < def->additionalPinsCapacity; i++) {
                    const AdditionalPinDef& pinDef = def->additionalPins[i];
                    data.additionalPins[i].id = pinDef.id;
                    
                    if(request->hasParam(pinDef.id, true)) {
                        data.additionalPins[i].gpio = request->getParam(pinDef.id, true)->value().toInt();
                    } else if(!pinDef.optional && pinDef.defaultValue != 255) {
                        data.additionalPins[i].gpio = pinDef.defaultValue;
                    } else {
                        data.additionalPins[i].gpio = pinDef.defaultValue;  /* 255 pour non connecté */
                    }
                }
                
                /* Allouer et remplir formFields */
                data.formFieldCount = def->formFieldCount;
                if(data.formFieldCount > 0) {
                    data.formFields = new ComplexComponentData::FormFieldValue[data.formFieldCount];
                    uint8_t fieldIndex = 0;
                    for(uint8_t i = 0; i < def->formFieldCount && i < def->formFieldsCapacity && fieldIndex < data.formFieldCount; i++) {
                        const FormFieldDef& field = def->formFields[i];
                        if(field.id && strlen(field.id) > 0 && field.id[0] != '_') {
                            data.formFields[fieldIndex].id = field.id;
                            if(request->hasParam(field.id, true)) {
                                data.formFields[fieldIndex].value = request->getParam(field.id, true)->value();
                            } else if(field.defaultValue && strlen(field.defaultValue) > 0) {
                                data.formFields[fieldIndex].value = String(field.defaultValue);
                            } else {
                                data.formFields[fieldIndex].value = "";
                            }
                            fieldIndex++;
                        }
                    }
                    data.formFieldCount = fieldIndex;  /* Ajuster le count réel */
                } else {
                    data.formFields = nullptr;
                }
                
                /* Allouer et remplir midiParams */
                data.midiParamCount = 0;
                data.midiParams = nullptr;
                if(def && def->midiMessageCount > 0 && def->midiMessages) {
                    /* Compter les paramètres MIDI */
                    for(uint8_t i = 0; i < def->midiMessageCount; i++) {
                        const MidiMessageDef& msg = def->midiMessages[i];
                        if(msg.params && msg.paramCount > 0) {
                            data.midiParamCount += msg.paramCount;
                        }
                    }
                    
                    if(data.midiParamCount > 0) {
                        data.midiParams = new ComplexComponentData::MidiParamValue[data.midiParamCount];
                        uint8_t paramIndex = 0;
                        for(uint8_t i = 0; i < def->midiMessageCount; i++) {
                            const MidiMessageDef& msg = def->midiMessages[i];
                            if(msg.params && msg.paramCount > 0) {
                                for(uint8_t j = 0; j < msg.paramCount && j < msg.paramsCapacity && paramIndex < data.midiParamCount; j++) {
                                    const MidiParamDef& param = msg.params[j];
                                    if(param.id && strlen(param.id) > 0) {
                                        data.midiParams[paramIndex].id = param.id;
                                        if(request->hasParam(param.id, true)) {
                                            data.midiParams[paramIndex].value = request->getParam(param.id, true)->value();
                                        } else if(param.defaultValue && strlen(param.defaultValue) > 0) {
                                            data.midiParams[paramIndex].value = String(param.defaultValue);
                                        } else {
                                            data.midiParams[paramIndex].value = "";
                                        }
                                        paramIndex++;
                                    }
                                }
                            }
                        }
                        data.midiParamCount = paramIndex;  /* Ajuster le count réel */
                    }
                }
                
                /* Lire paramètres OSC/Debug */
                data.oscEnabled = request->hasParam("oscEnabled", true) && request->getParam("oscEnabled", true)->value() == "true";
                data.oscAddress = request->hasParam("oscAddress", true) ? request->getParam("oscAddress", true)->value() : "";
                data.oscFormat = request->hasParam("oscFormat", true) ? request->getParam("oscFormat", true)->value() : "float";
                data.dbgEnabled = request->hasParam("dbgEnabled", true) && request->getParam("dbgEnabled", true)->value() == "true";
                data.dbgHeader = request->hasParam("dbgHeader", true) ? request->getParam("dbgHeader", true)->value() : "";
                
                /* VALIDATION AVANT d'appeler addComponent() */
                auto validation = ValidationRegistry::validateComplex(role.c_str(), data);
                if(!validation.valid) {
                    /* Libérer la mémoire allouée */
                    if(data.additionalPins) delete[] data.additionalPins;
                    if(data.formFields) delete[] data.formFields;
                    if(data.midiParams) delete[] data.midiParams;
                    
                    request->send(400, "application/json", 
                        "{\"status\":\"error\",\"message\":\"" + validation.error_message + "\"}");
                    return;
                }
                
                /* Appeler le handler générique */
                if(handler->addComponent(data)) {
                    /* Composant complexe ajouté avec succès */
                } else {
                    /* Libérer la mémoire allouée */
                    delete[] data.additionalPins;
                    if(data.formFields) delete[] data.formFields;
                    if(data.midiParams) delete[] data.midiParams;
                    request->send(500, "application/json", "{\"status\":\"error\",\"error\":\"Failed to add complex component\"}");
                    return;
                }
                
                /* Libérer la mémoire allouée */
                delete[] data.additionalPins;
                if(data.formFields) delete[] data.formFields;
                if(data.midiParams) delete[] data.midiParams;
            }
        }
        
        /* Mettre à jour ConfigCache */
        g_configCache.setConfigClean(pinLabel, json);
        nidmi_requestReloadPins();
        
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    /* API - Suppression d'une pin (unifié pour simples et complexes) */
    server.on("/api/pins/delete", HTTP_POST, [](AsyncWebServerRequest *request){
        if(request->hasParam("pin", true)){
            String pinLabel = request->getParam("pin", true)->value();
            
            /* Chercher le complexId depuis MuxManager en premier (plus fiable) */
            PinMapper::detectMcu();
            const PinMapping* mappings = PinMapper::getAllMappings();
            size_t mapping_count = PinMapper::getMappingCount();
            
            /* Trouver le GPIO SIG correspondant au pinLabel */
            uint8_t sigGpio = 255;
            for (size_t i = 0; i < mapping_count; i++) {
                if(String(mappings[i].label) == pinLabel) {
                    sigGpio = mappings[i].gpio;
                    break;
                }
            }
            
            /* Chercher si c'est un composant avec additionalPins en lisant la config NVS */
            Preferences preferences;
            preferences.begin("nidmi", true);
            String key = "pin_" + pinLabel;
            String configStr = preferences.getString(key.c_str(), "");
            preferences.end();
            
            /* Extraire le role depuis la config NVS si disponible */
            String role = "";
            if(configStr.length() > 0) {
                int roleStart = configStr.indexOf("\"role\":\"");
                if(roleStart >= 0) {
                    roleStart += 8;  /* Longueur de "\"role\":\"" */
                    int roleEnd = configStr.indexOf("\"", roleStart);
                    if(roleEnd > roleStart) {
                        role = configStr.substring(roleStart, roleEnd);
                    }
                }
            }
            
            /* Si role trouvé et handler disponible, utiliser le handler générique */
            if(role.length() > 0) {
                ComplexHandler* handler = ComplexHandlerRegistry::getHandler(role.c_str());
                if(handler && sigGpio != 255) {
                    /* Utiliser le handler générique pour supprimer le composant */
                    handler->removeComponent(pinLabel.c_str(), sigGpio);
                }
            } else {
                /* Pour les composants simples, supprimer via ComponentManager */
                if(sigGpio != 255) {
                    g_componentManager.removeComponent(sigGpio);
                }
            }
            
            /* Supprimer dans NVS et ConfigCache (pour tous les types) */
            g_configCache.removeConfig(pinLabel);
            
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            request->send(400, "application/json", "{\"error\":\"pin required\"}");
        }
    });
}
