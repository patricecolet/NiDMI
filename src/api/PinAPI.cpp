#include "APICommon.h"
#include "PinMapper.h"
#include "ComponentManager.h"
#include "Esp32Server.h" /* Pour esp32server_requestReloadPins */

/* Forward declarations */
String getDefaultConfig(String pin);
extern ComponentManager g_componentManager;

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

    /* API - Liste des pins configurées */
    server.on("/api/pins/configured", HTTP_GET, [](AsyncWebServerRequest *request){
        debug_network( "[PinAPI] /api/pins/configured appelé\n");
        String json = "{";
        json += "\"pins\":[";
        
        Preferences preferences;
        preferences.begin("esp32server", true);
        
        /* Scanner toutes les clés pin_* - Utiliser PinMapper pour obtenir dynamiquement toutes les pins */
        bool first = true;
        int pinCount = 0;
        
        /* Obtenir toutes les pins disponibles pour ce MCU */
        PinMapper::detectMcu();
        const PinMapping* mappings = PinMapper::getAllMappings();
        size_t mapping_count = PinMapper::getMappingCount();
        
        for (size_t i = 0; i < mapping_count; i++) {
            String pinLabel = String(mappings[i].label);
            String key = "pin_" + pinLabel;
            String config = preferences.getString(key.c_str(), "");
            if (!config.isEmpty()) {
                debug_network( "[PinAPI] Pin trouvée: %s -> %s\n", pinLabel.c_str(), config.c_str());
                if (!first) json += ",";
                json += "{\"pin\":\"" + pinLabel + "\",\"config\":" + config + "}";
                first = false;
                pinCount++;
            }
        }
        
        preferences.end();
        json += "]}";
        
        debug_network( "[PinAPI] Retourne %d pins configurées\n", pinCount);
        debug_network( "[PinAPI] JSON: %s\n", json.c_str());
        
        request->send(200, "application/json", json);
    });

    /* API - Configuration d'une pin */
    server.on("/api/pins/config", HTTP_POST, [](AsyncWebServerRequest *request){
        if(request->hasParam("pin", true) && request->hasParam("config", true)){
            String pin = request->getParam("pin", true)->value();
            String config = request->getParam("config", true)->value();
            
            /* Sauvegarder en NVS */
            Preferences preferences;
            preferences.begin("esp32server", false);
            String key = "pin_" + pin;
            preferences.putString(key.c_str(), config);
            preferences.end();
            
            request->send(200, "application/json", "{\"status\":\"ok\"}\n");
        } else {
            request->send(400, "application/json", "{\"error\":\"pin and config required\"}\n");
        }
    });

    /* API - Suppression d'une pin */
    server.on("/api/pins/delete", HTTP_POST, [](AsyncWebServerRequest *request){
        if(request->hasParam("pin", true)){
            String pin = request->getParam("pin", true)->value();
            g_configCache.removeConfig(pin);
            request->send(200, "application/json", "{\"status\":\"ok\"}\n");
        } else {
            request->send(400, "application/json", "{\"error\":\"pin required\"}\n");
        }
    });
    
    // ========================================================================
    // API - Multiplexeurs analogiques
    // ========================================================================
    
    /* API - Liste des multiplexeurs configurés */
    server.on("/api/mux/list", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = "{\"muxes\":[";
        bool first = true;
        
        Preferences prefs;
        prefs.begin("esp32server", true);
        
        for (uint8_t i = 0; i < MAX_MUXES; i++) {
            const MuxConfig* cfg = g_componentManager.getMuxConfig(i);
            if (cfg && cfg->enabled) {
                if (!first) json += ",";
                json += "{";
                json += "\"id\":" + String(i) + ",";
                json += "\"sig\":" + String(cfg->sig_pin) + ",";
                json += "\"s0\":" + String(cfg->s0) + ",";
                json += "\"s1\":" + String(cfg->s1) + ",";
                json += "\"s2\":" + String(cfg->s2) + ",";
                json += "\"s3\":" + String(cfg->s3) + ",";
                json += "\"en\":" + String(cfg->en_pin) + ",";
                // Retourner min/max du premier canal (interface web simplifiée)
                // Si tous les canaux ont la même valeur, retourner cette valeur unique
                uint16_t min_val = cfg->analog_min[0];
                uint16_t max_val = cfg->analog_max[0];
                json += "\"min\":" + String(min_val) + ",";
                json += "\"max\":" + String(max_val) + ",";
                json += "\"filterIntensity\":" + String(cfg->filter_intensity) + ",";
                // Format OSC : "raw", "float", "midi"
                String oscFormatStr = "float";
                if (cfg->osc_format == MuxOSCFormat::RAW) {
                    oscFormatStr = "raw";
                } else if (cfg->osc_format == MuxOSCFormat::MIDI) {
                    oscFormatStr = "midi";
                }
                json += "\"oscFormat\":\"" + oscFormatStr + "\"";
                
                /* Extraire oscBase, ccBase, midiChan depuis la première pin MUX (M0_0, M1_0) */
                String pinLabel = "M" + String(i) + "_0";
                String pinKey = "pin_" + pinLabel;
                if (prefs.isKey(pinKey.c_str())) {
                    String pinConfig = prefs.getString(pinKey.c_str(), "");
                    if (pinConfig.length() > 0) {
                        /* Extraire oscAddress et reconstruire oscBase */
                        int oscAddrStart = pinConfig.indexOf("\"oscAddress\":\"");
                        if (oscAddrStart >= 0) {
                            oscAddrStart += 14; /* Longueur de "oscAddress":" */
                            int oscAddrEnd = pinConfig.indexOf("\"", oscAddrStart);
                            if (oscAddrEnd > oscAddrStart) {
                                String oscAddr = pinConfig.substring(oscAddrStart, oscAddrEnd);
                                /* Reconstruire oscBase : enlever /0 ou /ch à la fin */
                                String oscBase = oscAddr;
                                /* Chercher le dernier / et enlever tout ce qui suit */
                                int lastSlash = oscBase.lastIndexOf('/');
                                if (lastSlash > 0) {
                                    oscBase = oscBase.substring(0, lastSlash);
                                } else if (oscBase.endsWith("0")) {
                                    /* Si c'est /mux0 sans slash final, enlever le 0 */
                                    oscBase = oscBase.substring(0, oscBase.length() - 1);
                                }
                                /* Si oscBase est /mux, reconstruire avec le numéro du MUX */
                                if (oscBase == "/mux") {
                                    oscBase = "/mux" + String(i);
                                }
                                json += ",\"oscBase\":\"" + oscBase + "\"";
                            }
                        }
                        
                        /* Extraire rtpCc pour ccBase */
                        int ccStart = pinConfig.indexOf("\"rtpCc\":");
                        if (ccStart >= 0) {
                            ccStart += 8;
                            int ccEnd = pinConfig.indexOf(",", ccStart);
                            if (ccEnd < 0) ccEnd = pinConfig.indexOf("}", ccStart);
                            if (ccEnd > ccStart) {
                                String ccStr = pinConfig.substring(ccStart, ccEnd);
                                json += ",\"ccBase\":" + ccStr;
                            }
                        }
                        
                        /* Extraire rtpChan pour midiChan */
                        int chanStart = pinConfig.indexOf("\"rtpChan\":");
                        if (chanStart >= 0) {
                            chanStart += 10;
                            int chanEnd = pinConfig.indexOf(",", chanStart);
                            if (chanEnd < 0) chanEnd = pinConfig.indexOf("}", chanStart);
                            if (chanEnd > chanStart) {
                                String chanStr = pinConfig.substring(chanStart, chanEnd);
                                json += ",\"midiChan\":" + chanStr;
                            }
                        }
                    }
                }
                
                json += "}";
                first = false;
            }
        }
        
        prefs.end();
        
        json += "],\"count\":" + String(g_componentManager.getMuxCount()) + "}";
        request->send(200, "application/json", json);
    });
    
    /* API - Ajouter/modifier un multiplexeur */
    server.on("/api/mux/add", HTTP_POST, [](AsyncWebServerRequest *request){
        if (!request->hasParam("id", true) || !request->hasParam("sig", true) ||
            !request->hasParam("s0", true) || !request->hasParam("s1", true) ||
            !request->hasParam("s2", true) || !request->hasParam("s3", true)) {
            request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Missing parameters\"}\n");
            return;
        }
        
        uint8_t mux_id = request->getParam("id", true)->value().toInt();
        uint8_t sig = request->getParam("sig", true)->value().toInt();
        uint8_t s0 = request->getParam("s0", true)->value().toInt();
        uint8_t s1 = request->getParam("s1", true)->value().toInt();
        uint8_t s2 = request->getParam("s2", true)->value().toInt();
        uint8_t s3 = request->getParam("s3", true)->value().toInt();
        uint8_t en = request->hasParam("en", true) ? request->getParam("en", true)->value().toInt() : 255;
        
        // Nouveaux paramètres MIDI/OSC (optionnels avec valeurs par défaut)
        uint8_t ccBase = request->hasParam("ccBase", true) ? request->getParam("ccBase", true)->value().toInt() : 1;
        uint8_t midiChan = request->hasParam("midiChan", true) ? request->getParam("midiChan", true)->value().toInt() : 1;
        String oscBase = request->hasParam("oscBase", true) ? request->getParam("oscBase", true)->value() : "/mux" + String(mux_id);
        
        // Paramètres seuils et hystérésis (optionnels avec valeurs par défaut)
        uint16_t analog_min = request->hasParam("min", true) ? request->getParam("min", true)->value().toInt() : 0;
        uint16_t analog_max = request->hasParam("max", true) ? request->getParam("max", true)->value().toInt() : 4095;
        // Hystérésis toujours activée (paramètre retiré de l'interface)
        bool hysteresis_enabled = true;
        
        // Format OSC (optionnel, défaut: FLOAT)
        MuxOSCFormat osc_format = MuxOSCFormat::FLOAT;
        if (request->hasParam("oscFormat", true)) {
            String oscFormatStr = request->getParam("oscFormat", true)->value();
            if (oscFormatStr == "raw") {
                osc_format = MuxOSCFormat::RAW;
            } else if (oscFormatStr == "midi") {
                osc_format = MuxOSCFormat::MIDI;
            } else {
                osc_format = MuxOSCFormat::FLOAT;
            }
        }
        
        // Intensité du filtrage (optionnel, défaut: 5)
        uint8_t filter_intensity = request->hasParam("filterIntensity", true) ? 
            request->getParam("filterIntensity", true)->value().toInt() : 5;
        if (filter_intensity < 1) filter_intensity = 1;
        if (filter_intensity > 10) filter_intensity = 10;
        
        // Valider les valeurs
        if (mux_id >= MAX_MUXES) {
            request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Invalid mux ID (0-" + String(MAX_MUXES - 1) + ")\"}\n");
            return;
        }
        
        if (ccBase > 127) ccBase = 127;
        if (midiChan < 1 || midiChan > 16) midiChan = 1;
        
        // Valider les seuils
        if (analog_min >= analog_max) {
            request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Invalid thresholds: min >= max\"}\n");
            return;
        }
        if (analog_max > 4095) {
            request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Invalid max threshold (max 4095)\"}\n");
            return;
        }
        
        if (g_componentManager.addMux(mux_id, sig, s0, s1, s2, s3, en, analog_min, analog_max, 
                                     hysteresis_enabled, osc_format, filter_intensity, oscBase.c_str())) {
            // Sauvegarder en NVS (config principale sans seuils)
            Preferences prefs;
            prefs.begin("esp32server", false);
            String key = "mux_" + String(mux_id);
            String config = String(sig) + "," + String(s0) + "," + String(s1) + "," + 
                           String(s2) + "," + String(s3) + "," + String(en) + "," +
                           String(hysteresis_enabled ? 1 : 0) + "," + String((int)osc_format) + "," +
                           String(filter_intensity) + "," + String(oscBase);
            prefs.putString(key.c_str(), config);
            
            // Sauvegarder les seuils en format binaire compact (uniform : 5 bytes)
            String thresh_key = "mux_thresh_" + String(mux_id);
            uint8_t buffer[5];
            buffer[0] = 0x01; // Flag: uniform = true
            buffer[1] = analog_min & 0xFF;
            buffer[2] = (analog_min >> 8) & 0xFF;
            buffer[3] = analog_max & 0xFF;
            buffer[4] = (analog_max >> 8) & 0xFF;
            prefs.putBytes(thresh_key.c_str(), buffer, 5);
            
            // Générer et sauvegarder les 16 configurations de pins MUX
            for (uint8_t ch = 0; ch < 16; ch++) {
                String pinLabel = "M" + String(mux_id) + "_" + String(ch);
                uint8_t cc = ccBase + ch;
                if (cc > 127) cc = 127; // Limiter à 127
                
                // Construire l'adresse OSC : utiliser oscBase directement (sans ajouter le canal)
                // Le canal sera envoyé comme float dans le message OSC
                String oscAddr = oscBase;
                
                // Construire la configuration JSON
                String pinConfig = "{";
                pinConfig += "\"role\":\"Potentiomètre\",";
                pinConfig += "\"rtpEnabled\":true,";
                pinConfig += "\"rtpType\":\"Control Change\",";
                pinConfig += "\"rtpCc\":" + String(cc) + ",";
                pinConfig += "\"rtpChan\":" + String(midiChan) + ",";
                pinConfig += "\"potFilter\":\"lowpass\",";
                pinConfig += "\"oscEnabled\":true,";
                pinConfig += "\"oscAddress\":\"" + oscAddr + "\",";
                pinConfig += "\"oscFormat\":\"float\",";
                pinConfig += "\"dbgEnabled\":false,";
                pinConfig += "\"dbgHeader\":\"\"";
                pinConfig += "}";
                
                // Sauvegarder en NVS
                String pinKey = "pin_" + pinLabel;
                prefs.putString(pinKey.c_str(), pinConfig);
                
                // Mettre en cache (sans marquer dirty car vient de NVS)
                g_configCache.setConfigClean(pinLabel, pinConfig);
            }
            
            prefs.end();
            
            request->send(200, "application/json", "{\"status\":\"ok\"}\n");
        } else {
            request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Failed to add mux\"}\n");
        }
    });
    
    /* API - Supprimer un multiplexeur */
    server.on("/api/mux/delete", HTTP_POST, [](AsyncWebServerRequest *request){
        if (!request->hasParam("id", true)) {
            request->send(400, "application/json", "{\"error\":\"id required\"}\n");
            return;
        }
        
        uint8_t mux_id = request->getParam("id", true)->value().toInt();
        
        if (g_componentManager.removeMux(mux_id)) {
            // Supprimer de NVS
            Preferences prefs;
            prefs.begin("esp32server", false);
            String key = "mux_" + String(mux_id);
            prefs.remove(key.c_str());
            
            // Supprimer les 16 configurations de pins MUX
            for (uint8_t ch = 0; ch < 16; ch++) {
                String pinLabel = "M" + String(mux_id) + "_" + String(ch);
                String pinKey = "pin_" + pinLabel;
                prefs.remove(pinKey.c_str());
                // Supprimer aussi du cache
                g_configCache.removeConfig(pinLabel);
            }
            
            prefs.end();
            
            request->send(200, "application/json", "{\"status\":\"ok\"}\n");
        } else {
            request->send(400, "application/json", "{\"error\":\"Failed to remove mux\"}\n");
        }
    });
}
