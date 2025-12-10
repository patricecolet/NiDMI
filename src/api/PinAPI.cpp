#include "APICommon.h"
#include "PinMapper.h"
#include "Esp32Server.h" /* Pour esp32server_requestReloadPins */

/* Forward declaration pour getDefaultConfig */
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
            
            /* Supprimer du cache ET de la NVS */
            g_configCache.removeConfig(pin);
            
            /* Recharger les configs pins */
            esp32server_requestReloadPins();
            
            request->send(200, "application/json", "{\"status\":\"ok\"}\n");
        } else {
            request->send(400, "application/json", "{\"error\":\"pin required\"}\n");
        }
    });
}
