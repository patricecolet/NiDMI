#include "APICommon.h"
#include "../config/SystemConfig.h"
#include "../server/ServerCallbacks.h"

void setupSystemAPI(AsyncWebServer& server) {
    /* API - Obtenir les paramètres système */
    server.on("/api/system/get", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = "{";
        json += "\"touchEnabled\":" + String(SystemConfig::isTouchEnabled() ? "true" : "false");
        json += "}";
        request->send(200, "application/json", json);
    });
    
    /* API - Modifier les paramètres système */
    server.on("/api/system/set", HTTP_POST, [](AsyncWebServerRequest *request){
        if (!request->hasParam("touchEnabled", true)) {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"touchEnabled parameter required\"}");
            return;
        }
        
        String touchEnabledStr = request->getParam("touchEnabled", true)->value();
        bool touchEnabled = (touchEnabledStr == "true" || touchEnabledStr == "1");
        
        SystemConfig::setTouchEnabled(touchEnabled);
        
        // Recharger les composants pour appliquer le changement
        nidmi_requestReloadPins();
        
        request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Settings updated, components reloaded\"}");
    });

    /* API - Reset logiciel (comme appuyer sur le bouton reset) */
    server.on("/api/system/reboot", HTTP_POST, [](AsyncWebServerRequest *request){
        request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Reboot scheduled\"}");
        // Reboot différé côté loop (pour laisser partir la réponse HTTP)
        nidmi_requestReboot();
    });
}
