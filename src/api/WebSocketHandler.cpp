#include <Arduino.h>

/*
void setupWebSocketHandler(AsyncWebSocket& ws) {
    debug_network( "[DEBUG] setupWebSocketHandler appelé\n");
    ws.onEvent(onWsEvent);
    debug_network( "[DEBUG] WebSocket onEvent configuré\n");
}
*/
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("[DEBUG] WebSocket client connected\n");
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("[DEBUG] WebSocket client disconnected\n");
    } else if (type == WS_EVT_DATA) {
        String message = String((char*)data);
        Serial.printf("[DEBUG] WebSocket message reçu: %s\n", message.c_str());
        
        if (message.startsWith("PIN_CLICKED:")) {
            String pin = message.substring(12);
            Serial.printf("[DEBUG] PIN_CLICKED reçu pour: %s\n", pin.c_str());
            
            /* Utiliser le cache (qui gère automatiquement NVS + defaults) */
            String config = g_configCache.getConfig(pin);
            Serial.printf("[DEBUG] Config retournée par cache: %s\n", config.c_str());
            
            String msg = "PIN_CONFIG:" + pin + ":" + config;
            Serial.printf("[DEBUG] Message WebSocket envoyé: %s\n", msg.c_str());
            client->text(msg);
        } else if (message.startsWith("PIN_CACHE:")) {
            Serial.printf("[DEBUG] PIN_CACHE reçu: %s\n", message.c_str());
            /* Format: PIN_CACHE:A0:{"role":"Potentiomètre",...} */
            int firstColon = message.indexOf(':', 10);
            if (firstColon > 10) {
                String pin = message.substring(10, firstColon);
                String config = message.substring(firstColon + 1);
                Serial.printf("[DEBUG] PIN_CACHE pin=%s, config=%s\n", pin.c_str(), config.c_str());
                
                /* Stocker en cache (pas en NVS) */
                g_configCache.setConfig(pin, config);
                g_configCache.autoSave(); /* Trigger auto-save check */
                Serial.printf("[DEBUG] PIN_CACHE sauvegardé en cache\n\n");
            }
        }
    }
}
