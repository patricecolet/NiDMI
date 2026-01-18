#include "APICommon.h"
#include "../utils/PinMapper.h"
#include "../managers/ComponentManager.h"
#include "../server/ServerCallbacks.h" /* Pour nidmi_requestReloadPins */
#include "../hardware/MuxConstants.h"
#include "../Globals.h"
#include "../config/ConfigCache.h"
#include "../components/ComponentRegistry.h"

/* Forward declarations */
String getDefaultConfig(String pin);

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
                        json += configStr;  // Le config est déjà un JSON complet avec pinLabel
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
                        
                        /* Construire le JSON unifié avec additionalPins */
                        json += "{";
                        json += "\"pinLabel\":\"" + sigPinLabel + "\",";
                        json += "\"role\":\"" + role + "\",";
                        json += "\"complexId\":" + String(i) + ",";
                        /* Construire additionalPins dynamiquement depuis la définition du composant */
                        const ComponentDefinition* compDef = ComponentRegistry::findById(role.c_str());
                        json += "\"additionalPins\":{";
                        if(compDef && compDef->additionalPinCount > 0 && compDef->additionalPins) {
                            bool firstPin = true;
                            /* Construire avec les valeurs du MuxConfig (spécifique MUX pour l'instant) */
                            for(uint8_t j = 0; j < compDef->additionalPinCount && j < compDef->additionalPinsCapacity; j++) {
                                if(!firstPin) json += ",";
                                String pinId = String(compDef->additionalPins[j].id);
                                /* Mapper les IDs des pins MUX vers les valeurs dans MuxConfig */
                                uint8_t pinValue = 255;
                                if(pinId == "sig") pinValue = cfg->sig_pin;
                                else if(pinId == "s0") pinValue = cfg->s0;
                                else if(pinId == "s1") pinValue = cfg->s1;
                                else if(pinId == "s2") pinValue = cfg->s2;
                                else if(pinId == "s3") pinValue = cfg->s3;
                                else if(pinId == "en") pinValue = cfg->en_pin;
                                json += "\"" + pinId + "\":" + String(pinValue);
                                firstPin = false;
                            }
                        } else {
                            /* Fallback pour compatibilité : construire avec les valeurs hardcodées MUX */
                            json += "\"sig\":" + String(cfg->sig_pin) + ",";
                            json += "\"s0\":" + String(cfg->s0) + ",";
                            json += "\"s1\":" + String(cfg->s1) + ",";
                            json += "\"s2\":" + String(cfg->s2) + ",";
                            json += "\"s3\":" + String(cfg->s3) + ",";
                            json += "\"en\":" + String(cfg->en_pin);
                        }
                        json += "},";
                        /* FormFields */
                        uint16_t min_val = cfg->analog_min[0];
                        uint16_t max_val = cfg->analog_max[0];
                        json += "\"min\":" + String(min_val) + ",";
                        json += "\"max\":" + String(max_val) + ",";
                        json += "\"filterIntensity\":" + String(cfg->filter_intensity) + ",";
                        /* MIDI/OSC config : mapper ccBase → rtpCc, midiChan → rtpChan, oscBase → oscAddress */
                        json += "\"rtpCc\":" + String(cfg->cc_base) + ",";
                        json += "\"rtpChan\":" + String(cfg->midi_channel) + ",";
                        json += "\"rtpEnabled\":true,";
                        String oscBase = (cfg->osc_base[0] != '\0') ? String(cfg->osc_base) : "/mux" + String(i);
                        json += "\"oscAddress\":\"" + oscBase + "\",";
                        String oscFormatStr = "float";
                        if (cfg->osc_format == MuxOSCFormat::RAW) {
                            oscFormatStr = "raw";
                        } else if (cfg->osc_format == MuxOSCFormat::MIDI) {
                            oscFormatStr = "midi";
                        }
                        json += "\"oscFormat\":\"" + oscFormatStr + "\"";
                        json += "}";
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
        
        /* Construire le JSON à partir des paramètres */
        String json = "{";
        json += "\"pinLabel\":\"" + pinLabel + "\",";
        json += "\"role\":\"" + role + "\"";
        
        auto addParam = [&](const char* name) {
            if(request->hasParam(name, true)) {
                String val = request->getParam(name, true)->value();
                if(val == "true" || val == "false") {
                    json += ",\"" + String(name) + "\":" + val;
                } else if(val.length() > 0 && (val[0] >= '0' && val[0] <= '9')) {
                    json += ",\"" + String(name) + "\":" + val;
                } else {
                    json += ",\"" + String(name) + "\":\"" + val + "\"";
                }
            }
        };
        
        /* Paramètres MIDI communs (toujours présents) */
        addParam("rtpEnabled");
        addParam("rtpType");
        
        /* Lire dynamiquement les paramètres MIDI depuis def->midiMessages[].params[] */
        if(def && def->midiMessageCount > 0 && def->midiMessages) {
            for(uint8_t i = 0; i < def->midiMessageCount; i++) {
                const MidiMessageDef& msg = def->midiMessages[i];
                if(msg.params && msg.paramCount > 0) {
                    for(uint8_t j = 0; j < msg.paramCount && j < msg.paramsCapacity; j++) {
                        const MidiParamDef& param = msg.params[j];
                        if(param.id && strlen(param.id) > 0) {
                            addParam(param.id);
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
                    if(!request->hasParam(def->additionalPins[i].id, true)) {
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
                
                /* Ajouter complexId si présent */
                if(request->hasParam("complexId", true)) {
                    json += ",\"complexId\":" + request->getParam("complexId", true)->value();
                }
            }
        }
        
        json += "}";
        
        /* Sauvegarder en NVS */
        Preferences preferences;
        preferences.begin("nidmi", false);
        String key = "pin_" + pinLabel;
        preferences.putString(key.c_str(), json);
        
        /* Si composant avec additionalPins, sauvegarder aussi le pinLabel et le rôle pour référence future */
        if(hasAdditionalPins && request->hasParam("complexId", true)) {
            String complexIdStr = request->getParam("complexId", true)->value();
            String keyPinLabel = "pinLabel_complex_" + complexIdStr;
            preferences.putString(keyPinLabel.c_str(), pinLabel);
            /* Stocker le rôle pour pouvoir le retrouver lors du chargement */
            String roleKey = "role_complex_" + complexIdStr;
            preferences.putString(roleKey.c_str(), role);
        }
        
        preferences.end();
        
        /* Si additionalPins présent, sauvegarder aussi dans le manager approprié (MuxManager pour les MUX) */
        if(hasAdditionalPins && def) {
            /* Lire dynamiquement les additionalPins depuis la requête */
            /* D'abord, vérifier si c'est un composant MUX (hc4067, hc4051) */
            bool isMuxComponent = (role == "hc4067" || role == "hc4051");
            
            if(isMuxComponent) {
                /* Pour les MUX, lire les pins depuis les additionalPins dynamiquement */
                uint8_t sig = 255;
                uint8_t s0 = 255, s1 = 255, s2 = 255, s3 = 255;
                uint8_t en = 255;
                
                /* Lire dynamiquement depuis les additionalPins de la définition */
                for(uint8_t i = 0; i < def->additionalPinCount && i < def->additionalPinsCapacity; i++) {
                    const AdditionalPinDef& pinDef = def->additionalPins[i];
                    String pinId = String(pinDef.id);
                    
                    if(request->hasParam(pinId.c_str(), true)) {
                        uint8_t value = request->getParam(pinId.c_str(), true)->value().toInt();
                        /* Mapper vers les variables locales pour addMux() */
                        if(pinId == "sig") sig = value;
                        else if(pinId == "s0") s0 = value;
                        else if(pinId == "s1") s1 = value;
                        else if(pinId == "s2") s2 = value;
                        else if(pinId == "s3") s3 = value;
                        else if(pinId == "en") en = value;
                    } else if(!pinDef.optional && pinDef.defaultValue != 255) {
                        /* Pin requise absente, utiliser valeur par défaut */
                        if(pinId == "sig") sig = pinDef.defaultValue;
                        else if(pinId == "s0") s0 = pinDef.defaultValue;
                        else if(pinId == "s1") s1 = pinDef.defaultValue;
                        else if(pinId == "s2") s2 = pinDef.defaultValue;
                        else if(pinId == "s3") s3 = pinDef.defaultValue;
                        else if(pinId == "en") en = pinDef.defaultValue;
                    }
                }
                
                /* Vérifier que les pins requises sont présentes */
                bool hasRequiredPins = (sig != 255 && s0 != 255 && s1 != 255 && s2 != 255);
                /* s3 peut être absent pour HC4051 (3 bits seulement) */
                if(role == "hc4051") {
                    hasRequiredPins = (sig != 255 && s0 != 255 && s1 != 255 && s2 != 255);
                } else {
                    hasRequiredPins = hasRequiredPins && (s3 != 255);
                }
                
                if(!hasRequiredPins) {
                    request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Missing required additionalPins\"}");
                    return;
                }
                
                /* Utiliser valeurs par défaut pour s3 si absent (HC4051) */
                if(role == "hc4051" && s3 == 255) {
                    s3 = 255; /* Pin s3 non utilisée pour HC4051 */
                }
                
                /* ID du composant complexe */
                uint8_t mux_id = request->hasParam("complexId", true) ? request->getParam("complexId", true)->value().toInt() : 0;
                /* Générer un ID disponible si non fourni */
                if(mux_id == 0) {
                    for(uint8_t i = 0; i < MAX_MUXES; i++) {
                        const MuxConfig* cfg = g_componentManager.getMuxConfig(i);
                        if(!cfg || !cfg->enabled) {
                            mux_id = i;
                            break;
                        }
                    }
                }
                
                /* MIDI/OSC config : mapper depuis rtpCc → ccBase, rtpChan → midiChan, oscAddress → oscBase */
                uint8_t ccBase = request->hasParam("rtpCc", true) ? request->getParam("rtpCc", true)->value().toInt() : 1;
                uint8_t midiChan = request->hasParam("rtpChan", true) ? request->getParam("rtpChan", true)->value().toInt() : 1;
                String oscBase = request->hasParam("oscAddress", true) ? request->getParam("oscAddress", true)->value() : "/mux" + String(mux_id);
                
                /* FormFields : min, max, filterIntensity */
                uint16_t analog_min = request->hasParam("min", true) ? request->getParam("min", true)->value().toInt() : 0;
                uint16_t analog_max = request->hasParam("max", true) ? request->getParam("max", true)->value().toInt() : 4095;
                uint8_t filter_intensity = request->hasParam("filterIntensity", true) ? request->getParam("filterIntensity", true)->value().toInt() : 5;
                if(filter_intensity < 1) filter_intensity = 1;
                if(filter_intensity > 10) filter_intensity = 10;
                
                /* Format OSC */
                MuxOSCFormat osc_format = MuxOSCFormat::FLOAT;
                if(request->hasParam("oscFormat", true)) {
                    String oscFormatStr = request->getParam("oscFormat", true)->value();
                    if(oscFormatStr == "raw") {
                        osc_format = MuxOSCFormat::RAW;
                    } else if(oscFormatStr == "midi") {
                        osc_format = MuxOSCFormat::MIDI;
                    }
                }
                
                /* Valider les valeurs */
                if(mux_id >= MAX_MUXES) {
                    request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Invalid complex ID (0-" + String(MAX_MUXES - 1) + ")\"}");
                    return;
                }
                
                /* Sauvegarder dans MuxManager */
                if(g_componentManager.addMux(mux_id, sig, s0, s1, s2, s3, en, analog_min, analog_max,
                                             true, osc_format, filter_intensity, ccBase, midiChan, oscBase.c_str())) {
                    /* Sauvegarder aussi les clés NVS spécifiques aux mux (mux_X, mux_thresh_X) pour persistance */
                    Preferences prefs;
                    prefs.begin("nidmi", false);
                    String mux_key = "mux_" + String(mux_id);
                    String mux_config = String(sig) + "," + String(s0) + "," + String(s1) + "," +
                                       String(s2) + "," + String(s3) + "," + String(en) + "," +
                                       String(ccBase) + "," + String(midiChan) + "," +
                                       "1," + String((int)osc_format) + "," +
                                       String(filter_intensity) + "," + String(oscBase);
                    prefs.putString(mux_key.c_str(), mux_config);
                    
                    /* Sauvegarder les seuils */
                    String thresh_key = "mux_thresh_" + String(mux_id);
                    uint8_t buffer[5];
                    buffer[0] = 0x01;
                    buffer[1] = analog_min & 0xFF;
                    buffer[2] = (analog_min >> 8) & 0xFF;
                    buffer[3] = analog_max & 0xFF;
                    buffer[4] = (analog_max >> 8) & 0xFF;
                    prefs.putBytes(thresh_key.c_str(), buffer, 5);
                    prefs.end();
                }
            }
            /* Ici, on peut ajouter d'autres types de composants complexes à l'avenir */
            /* Par exemple : else if(isOtherComponentType) { ... } */
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
            
            /* Chercher dans MuxManager si un MUX utilise ce GPIO comme SIG */
            uint8_t foundMuxId = 255;
            if(sigGpio != 255) {
                for(uint8_t i = 0; i < MAX_MUXES; i++) {
                    const MuxConfig* cfg = g_componentManager.getMuxConfig(i);
                    if(cfg && cfg->enabled && cfg->sig_pin == sigGpio) {
                        foundMuxId = i;
                        break;
                    }
                }
            }
            
            /* Si pas trouvé dans MuxManager, essayer depuis NVS (fallback) */
            if(foundMuxId == 255) {
                Preferences preferences;
                preferences.begin("nidmi", true);
                String key = "pin_" + pinLabel;
                String configStr = preferences.getString(key.c_str(), "");
                preferences.end();
                
                bool hasAdditionalPins = configStr.indexOf("\"additionalPins\"") >= 0;
                if(hasAdditionalPins) {
                    int complexIdStart = configStr.indexOf("\"complexId\":");
                    if(complexIdStart >= 0) {
                        int complexIdEnd = configStr.indexOf(",", complexIdStart);
                        if(complexIdEnd < 0) complexIdEnd = configStr.indexOf("}", complexIdStart);
                        if(complexIdEnd >= 0) {
                            String complexIdStr = configStr.substring(complexIdStart + 12, complexIdEnd);
                            foundMuxId = complexIdStr.toInt();
                        }
                    }
                }
            }
            
            /* Si on a trouvé un composant complexe, le supprimer */
            if(foundMuxId != 255) {
                /* Pour l'instant, on utilise encore MuxManager pour les MUX */
                if(g_componentManager.removeMux(foundMuxId)) {
                    /* Supprimer aussi les clés NVS (génériques et spécifiques MUX pour compatibilité) */
                    Preferences prefs;
                    prefs.begin("nidmi", false);
                    /* Clés génériques */
                    String pinLabelKey = "pinLabel_complex_" + String(foundMuxId);
                    prefs.remove(pinLabelKey.c_str());
                    String roleKey = "role_complex_" + String(foundMuxId);
                    prefs.remove(roleKey.c_str());
                    /* Clés spécifiques MUX (pour compatibilité et MuxManager) */
                    String muxKey = "mux_" + String(foundMuxId);
                    prefs.remove(muxKey.c_str());
                    String threshKey = "mux_thresh_" + String(foundMuxId);
                    prefs.remove(threshKey.c_str());
                    /* Clés anciennes pour compatibilité */
                    String oldPinLabelKey = "pinLabel_mux_" + String(foundMuxId);
                    prefs.remove(oldPinLabelKey.c_str());
                    String oldRoleKey = "role_mux_" + String(foundMuxId);
                    prefs.remove(oldRoleKey.c_str());
                    prefs.end();
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
