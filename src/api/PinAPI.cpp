#include "APICommon.h"
#include "../utils/PinMapper.h"
#include "../managers/ComponentManager.h"
#include "../server/ServerCallbacks.h" /* Pour nidmi_requestReloadPins */
#include "../hardware/MuxConstants.h"
#include "../Globals.h"
#include "../config/ConfigCache.h"

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
        
        for (size_t i = 0; i < mapping_count; i++) {
            String pinLabel = String(mappings[i].label);
            String key = "pin_" + pinLabel;
            String configStr = preferences.getString(key.c_str(), "");
            if (!configStr.isEmpty()) {
                if (!first) json += ",";
                json += configStr;  // Le config est déjà un JSON complet avec pinLabel
                first = false;
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
        
        addParam("rtpEnabled");
        addParam("rtpType");
        addParam("rtpNote");
        addParam("rtpCc");
        addParam("rtpPc");
        addParam("rtpChan");
        addParam("rtpCcOn");
        addParam("rtpCcOff");
        addParam("rtpVel");
        addParam("rtpCcMin");
        addParam("rtpCcMax");
        addParam("rtpNoteMin");
        addParam("rtpNoteMax");
        addParam("rtpNoteVelFix");
        addParam("rtpNoteSweepAutoOffDelay");
        addParam("ledMode");
        addParam("btnMode");
        addParam("btnPulseTiming");
        addParam("filterIntensity");
        addParam("oscEnabled");
        addParam("oscAddress");
        addParam("oscFormat");
        addParam("dbgEnabled");
        addParam("dbgHeader");
        
        json += "}";
        
        /* Sauvegarder en NVS */
        Preferences preferences;
        preferences.begin("nidmi", false);
        String key = "pin_" + pinLabel;
        preferences.putString(key.c_str(), json);
        preferences.end();
        
        /* Mettre à jour ConfigCache */
        g_configCache.setConfigClean(pinLabel, json);
        nidmi_requestReloadPins();
        
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    /* API - Suppression d'une pin */
    server.on("/api/pins/delete", HTTP_POST, [](AsyncWebServerRequest *request){
        if(request->hasParam("pin", true)){
            String pin = request->getParam("pin", true)->value();
            g_configCache.removeConfig(pin);
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            request->send(400, "application/json", "{\"error\":\"pin required\"}");
        }
    });
    
    // ========================================================================
    // API - Multiplexeurs analogiques
    // ========================================================================
    
    /* API - Liste des multiplexeurs configurés */
    server.on("/api/mux/list", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = "{\"muxes\":[";
        bool first = true;
        
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
                uint16_t min_val = cfg->analog_min[0];
                uint16_t max_val = cfg->analog_max[0];
                json += "\"min\":" + String(min_val) + ",";
                json += "\"max\":" + String(max_val) + ",";
                json += "\"filterIntensity\":" + String(cfg->filter_intensity) + ",";
                json += "\"ccBase\":" + String(cfg->cc_base) + ",";
                json += "\"midiChan\":" + String(cfg->midi_channel) + ",";
                
                String oscBase = (cfg->osc_base[0] != '\0') ? String(cfg->osc_base) : "/mux" + String(i);
                json += "\"oscBase\":\"" + oscBase + "\",";
                
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
        
        json += "],\"count\":" + String(g_componentManager.getMuxCount()) + "}";
        request->send(200, "application/json", json);
    });
    
    /* API - Ajouter/modifier un multiplexeur */
    server.on("/api/mux/add", HTTP_POST, [](AsyncWebServerRequest *request){
        if (!request->hasParam("id", true) || !request->hasParam("sig", true) ||
            !request->hasParam("s0", true) || !request->hasParam("s1", true) ||
            !request->hasParam("s2", true) || !request->hasParam("s3", true)) {
            request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Missing parameters\"}");
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
            request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Invalid mux ID (0-" + String(MAX_MUXES - 1) + ")\"}");
            return;
        }
        
        if (ccBase > 127) ccBase = 127;
        if (midiChan < 1 || midiChan > 16) midiChan = 1;
        
        // Valider les seuils
        if (analog_min >= analog_max) {
            request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Invalid thresholds: min >= max\"}");
            return;
        }
        if (analog_max > 4095) {
            request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Invalid max threshold (max 4095)\"}");
            return;
        }
        
        if (g_componentManager.addMux(mux_id, sig, s0, s1, s2, s3, en, analog_min, analog_max,
                                     hysteresis_enabled, osc_format, filter_intensity, ccBase, midiChan, oscBase.c_str())) {
            // Sauvegarder en NVS (config principale sans seuils)
            Preferences prefs;
            prefs.begin("nidmi", false);
            String key = "mux_" + String(mux_id);
            String config = String(sig) + "," + String(s0) + "," + String(s1) + "," +
                           String(s2) + "," + String(s3) + "," + String(en) + "," +
                           String(ccBase) + "," + String(midiChan) + "," +
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
            
            prefs.end();
            
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Failed to add mux\"}");
        }
    });
    
    /* API - Supprimer un multiplexeur */
    server.on("/api/mux/delete", HTTP_POST, [](AsyncWebServerRequest *request){
        if (!request->hasParam("id", true)) {
            request->send(400, "application/json", "{\"error\":\"id required\"}");
            return;
        }
        
        uint8_t mux_id = request->getParam("id", true)->value().toInt();
        
        if (g_componentManager.removeMux(mux_id)) {
            // Supprimer de NVS
            Preferences prefs;
            prefs.begin("nidmi", false);
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
            
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            request->send(400, "application/json", "{\"error\":\"Failed to remove mux\"}");
        }
    });
}
