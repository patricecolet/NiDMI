#include "../server/ServerCore.h"
#include "ui_index.h"
#include "ui_bundle.h"
#include "../utils/PinMapper.h"
#include "../api/APICommon.h"
#include "../api/NetworkAPI.h"
#include "../api/RTPAPI.h"
#include "../Globals.h"
#include "../server/ServerCallbacks.h"
#include "../components/ComponentRegistry.h"
#include <Preferences.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <pgmspace.h>

// Forward declarations pour les APIs
void setupPinAPI(AsyncWebServer& server);
void setupOSC_API(AsyncWebServer& server);
void setupCacheAPI(AsyncWebServer& server);
void setupComponentsAPI(AsyncWebServer& server);

Preferences preferences;

// Fonction pour obtenir la configuration par défaut d'une pin
String getDefaultConfig(String pin) {
    // Pins analogiques (A0, A1, A2, ... A10) - dynamique selon le MCU
    if (pin.startsWith("A")) {
        PinMapper::detectMcu();
        uint8_t gpio = PinMapper::labelToGpio(pin.c_str());
        if (gpio != 255 && PinMapper::hasAdc(gpio)) {
            // Extraire le numéro de la pin pour le CC par défaut
            int pinNum = pin.substring(1).toInt();
            int default_cc = pinNum + 1; // A0 -> CC1, A1 -> CC2, etc.
            if (default_cc > 127) default_cc = 127;
            
            String config = "{\"role\":\"potentiometer\",\"rtpEnabled\":true,\"rtpType\":\"Control Change\",";
            config += "\"rtpCc\":" + String(default_cc) + ",\"rtpChan\":1,";
            config += "\"filterIntensity\":5,\"oscEnabled\":true,\"oscAddress\":\"/ctl\",";
            config += "\"oscFormat\":\"float\",\"dbgEnabled\":false,\"dbgHeader\":\"\"}";
            return config;
        }
    }
    
    // Pins MUX (M0_0 à M1_15) - traiter comme potentiomètres
    if (pin.startsWith("M")) {
        // Extraire le numéro de mux et canal : M0_0 -> mux=0, ch=0
        int underscore_pos = pin.indexOf('_');
        if (underscore_pos > 0 && underscore_pos < pin.length() - 1) {
            int mux_id = pin.substring(1, underscore_pos).toInt();
            int channel = pin.substring(underscore_pos + 1).toInt();
            // CC par défaut : 1 + (mux_id * 16) + channel
            int default_cc = 1 + (mux_id * 16) + channel;
            if (default_cc > 127) default_cc = 127; // Limiter à 127
            
            String config = "{\"role\":\"potentiometer\",\"rtpEnabled\":true,\"rtpType\":\"Control Change\",";
            config += "\"rtpCc\":" + String(default_cc) + ",\"rtpChan\":1,";
            config += "\"filterIntensity\":5,\"oscEnabled\":true,\"oscAddress\":\"/ctl\",";
            config += "\"oscFormat\":\"float\",\"dbgEnabled\":false,\"dbgHeader\":\"\"}";
            return config;
        }
    }
    
    if (pin == "D0") return "{\"role\":\"button\",\"rtpEnabled\":true,\"rtpType\":\"Note\",\"rtpNote\":60,\"rtpChan\":1,\"btnMode\":\"pulse\",\"btnPulseTiming\":\"release\",\"oscEnabled\":true,\"oscAddress\":\"/note\",\"oscFormat\":\"float\",\"dbgEnabled\":false,\"dbgHeader\":\"\"}";
    if (pin == "D1") return "{\"role\":\"button\",\"rtpEnabled\":true,\"rtpType\":\"Note\",\"rtpNote\":61,\"rtpChan\":1,\"btnMode\":\"pulse\",\"btnPulseTiming\":\"release\",\"oscEnabled\":true,\"oscAddress\":\"/note\",\"oscFormat\":\"float\",\"dbgEnabled\":false,\"dbgHeader\":\"\"}";
    if (pin == "D2") return "{\"role\":\"button\",\"rtpEnabled\":true,\"rtpType\":\"Note\",\"rtpNote\":62,\"rtpChan\":1,\"btnMode\":\"pulse\",\"btnPulseTiming\":\"release\",\"oscEnabled\":true,\"oscAddress\":\"/note\",\"oscFormat\":\"float\",\"dbgEnabled\":false,\"dbgHeader\":\"\"}";
    if (pin == "D3") return "{\"role\":\"button\",\"rtpEnabled\":true,\"rtpType\":\"Note\",\"rtpNote\":63,\"rtpChan\":1,\"btnMode\":\"pulse\",\"btnPulseTiming\":\"release\",\"oscEnabled\":true,\"oscAddress\":\"/note\",\"oscFormat\":\"float\",\"dbgEnabled\":false,\"dbgHeader\":\"\"}";
    
    // LEDs spéciales
    if (pin == "D7") return "{\"role\":\"LED\",\"rtpEnabled\":true,\"rtpType\":\"Note\",\"rtpNote\":36,\"rtpChan\":1,\"ledMode\":\"onoff\",\"oscEnabled\":true,\"oscAddress\":\"/note\",\"dbgEnabled\":false,\"dbgHeader\":\"\"}";
    if (pin == "D8") return "{\"role\":\"LED\",\"rtpEnabled\":true,\"rtpType\":\"Note\",\"rtpNote\":37,\"rtpChan\":1,\"ledMode\":\"onoff\",\"oscEnabled\":true,\"oscAddress\":\"/note\",\"dbgEnabled\":false,\"dbgHeader\":\"\"}";
    if (pin == "D9") return "{\"role\":\"LED\",\"rtpEnabled\":true,\"rtpType\":\"Note\",\"rtpNote\":38,\"rtpChan\":1,\"ledMode\":\"onoff\",\"oscEnabled\":true,\"oscAddress\":\"/note\",\"dbgEnabled\":false,\"dbgHeader\":\"\"}";
    if (pin == "D10") return "{\"role\":\"LED\",\"rtpEnabled\":true,\"rtpType\":\"Control Change\",\"rtpCc\":10,\"rtpChan\":1,\"ledMode\":\"pwm\",\"oscEnabled\":true,\"oscAddress\":\"/ctl\",\"dbgEnabled\":false,\"dbgHeader\":\"\"}";
    
    // Bus
    if (pin == "SDA" || pin == "SCL") return "{\"role\":\"I2C\",\"rtpEnabled\":false,\"oscEnabled\":true,\"oscAddress\":\"/ctl\",\"dbgEnabled\":false,\"dbgHeader\":\"\"}";
    if (pin == "MOSI" || pin == "MISO" || pin == "SCK") return "{\"role\":\"SPI\",\"rtpEnabled\":false,\"oscEnabled\":true,\"oscAddress\":\"/ctl\",\"dbgEnabled\":false,\"dbgHeader\":\"\"}";
    if (pin == "TX" || pin == "RX") return "{\"role\":\"UART\",\"rtpEnabled\":false,\"oscEnabled\":true,\"oscAddress\":\"/ctl\",\"dbgEnabled\":false,\"dbgHeader\":\"\"}";
    
    // Défaut
    return "{\"role\":\"button\",\"rtpEnabled\":true,\"rtpType\":\"Note\",\"rtpNote\":60,\"rtpChan\":1,\"btnMode\":\"pulse\",\"btnPulseTiming\":\"release\",\"oscEnabled\":true,\"oscAddress\":\"/note\",\"dbgEnabled\":false,\"dbgHeader\":\"\"}";
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.println("WebSocket client connected");
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.println("WebSocket client disconnected");
    } else if (type == WS_EVT_DATA) {
        String message = String((char*)data);
        
        if (message.startsWith("PIN_CLICKED:")) {
            String pin = message.substring(12);
            
            // Vérifier NVS (compatible avec système existant)
            preferences.begin("nidmi", true);
            String key = "pin_" + pin;
            String config = preferences.getString(key.c_str(), "");
            preferences.end();
            
            if (config.length() > 0) {
                // Config trouvée → Envoyer config NVS
                String msg = "PIN_CONFIG:" + pin + ":" + config;
                client->text(msg);
            } else {
                // Pas de config → Envoyer valeurs par défaut complètes
                String defaultConfig = getDefaultConfig(pin);
                String msg = "PIN_CONFIG:" + pin + ":" + defaultConfig;
                client->text(msg);
            }
        }
    }
}

// Fonction pour envoyer le statut RTP-MIDI via WebSocket
void sendRtpStatus(AsyncWebSocket& ws) {
    preferences.begin("nidmi", false);
    bool enabled = preferences.getBool("rtp_enabled", false);
    String name = preferences.getString("rtp_name", "ESP32-Studio");
    String target = preferences.getString("rtp_target", "sta");
    preferences.end();
    
    bool connected = serverCore.rtpMidi().isConnected();
    
    String json = "{";
    json += "\"type\":\"rtp_status\",";
    json += "\"enabled\":" + String(enabled ? "true" : "false") + ",";
    json += "\"name\":\"" + name + "\",";
    json += "\"target\":\"" + target + "\",";
    json += "\"connected\":" + String(connected ? "true" : "false");
    json += "}";
    
    ws.textAll(json);
}

void setupWebAPI(AsyncWebServer& server, AsyncWebSocket& ws) {
    // Serial.println("[WebAPI] Starting setup...");
    
    // Page principale - Utiliser streaming par chunks depuis PROGMEM
    // Serial.println("[WebAPI] Setting up main page...");
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        size_t htmlLen = strlen_P(INDEX_HTML);
        AsyncWebServerResponse *response = request->beginResponse("text/html; charset=utf-8", htmlLen, 
            [htmlLen](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
                size_t toWrite = (htmlLen - index < maxLen) ? (htmlLen - index) : maxLen;
                if (toWrite > 0) {
                    memcpy_P(buffer, INDEX_HTML + index, toWrite);
                }
                return toWrite;
            });
        response->addHeader("Connection", "close");
        request->send(response);
    });
    
    // Bundle JavaScript compressé en gzip (route /bundle)
    server.on("/bundle", HTTP_GET, [](AsyncWebServerRequest *request){
        const char *type = "text/javascript";
        AsyncWebServerResponse *response = request->beginResponse_P(200, type, BUNDLE, BUNDLE_LEN);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    // WebSocket
    // Serial.println("[WebAPI] Setting up WebSocket...");
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
    
    // Initialiser le registre des composants
    ComponentRegistry::init();
    
    // Configurer les autres APIs
    setupNetworkAPI(server);
    setupRTPAPI(server);
    setupPinAPI(server);
    setupOSC_API(server);
    setupCacheAPI(server);
    setupComponentsAPI(server);
    
    // Serial.println("[WebAPI] Setup complete!");
}
