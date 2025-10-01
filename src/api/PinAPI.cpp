#include "APICommon.h"
#include "PinMapper.h"

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
            json += "\"touch\":" + String(mappings[i].has_touch ? "true" : "false\n");
            json += "},";
            json += "\"sensitive\":false";
            json += "}";
        }
        
        json += "],";
        json += "\"bus\":{";
        
        /* Bus I2C */
        json += "\"i2c\":{";
        if (PinMapper::getMcuType() == McuType::ESP32_C3) {
            json += "\"sda\":6,\"scl\":7";
        } else {
            json += "\"sda\":21,\"scl\":22";
        }
        json += "},";
        
        /* Bus SPI */
        json += "\"spi\":{";
        if (PinMapper::getMcuType() == McuType::ESP32_C3) {
            json += "\"mosi\":8,\"miso\":9,\"sck\":10";
        } else {
            json += "\"mosi\":23,\"miso\":19,\"sck\":18";
        }
        json += "},";
        
        /* Bus UART */
        json += "\"uart\":{";
        if (PinMapper::getMcuType() == McuType::ESP32_C3) {
            json += "\"tx\":21,\"rx\":20";
        } else {
            json += "\"tx\":1,\"rx\":3";
        }
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
        
        /* Scanner toutes les clés pin_* */
        bool first = true;
        int pinCount = 0;
        String pins[] = {"A0","A1","A2","A3","D0","D1","D2","D3","D4","D5","D6","D7","D8","D9","D10","SDA","SCL","TX","RX","MOSI","MISO","SCK"};
        
        for (int i = 0; i < 22; i++) {
            String key = "pin_" + pins[i];
            String config = preferences.getString(key.c_str(), "");
            if (!config.isEmpty()) {
                debug_network( "[PinAPI] Pin trouvée: %s -> %s\n", pins[i].c_str(), config.c_str());
                if (!first) json += ",";
                json += "{\"pin\":\"" + pins[i] + "\",\"config\":" + config + "}";
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
            
            /* Supprimer de la NVS */
            Preferences preferences;
            preferences.begin("esp32server", false);
            String key = "pin_" + pin;
            preferences.remove(key.c_str());
            preferences.end();
            
            request->send(200, "application/json", "{\"status\":\"ok\"}\n");
        } else {
            request->send(400, "application/json", "{\"error\":\"pin required\"}\n");
        }
    });
}
