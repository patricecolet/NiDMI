#include "APICommon.h"
#include "../config/SystemConfig.h"
#include "../server/ServerCallbacks.h"

void setupSystemAPI(AsyncWebServer& server) {
    /* API - Obtenir les paramètres système */
    server.on("/api/system/get", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = "{";
        json += "\"touchEnabled\":" + String(SystemConfig::isTouchEnabled() ? "true" : "false");
        json += ",\"componentsPerCycle\":" + String((int)SystemConfig::componentsPerCycle());
        json += "}";
        request->send(200, "application/json", json);
    });
    
    /* API - Modifier les paramètres système */
    server.on("/api/system/set", HTTP_POST, [](AsyncWebServerRequest *request){
        /* Les deux réglages sont indépendants : on accepte l'un, l'autre, ou les deux,
           pour ne pas casser les clients qui n'envoient que touchEnabled. */
        bool hasTouch = request->hasParam("touchEnabled", true);
        bool hasSlice = request->hasParam("componentsPerCycle", true);
        if (!hasTouch && !hasSlice) {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"touchEnabled or componentsPerCycle parameter required\"}");
            return;
        }

        if (hasTouch) {
            String touchEnabledStr = request->getParam("touchEnabled", true)->value();
            bool touchEnabled = (touchEnabledStr == "true" || touchEnabledStr == "1");
            SystemConfig::setTouchEnabled(touchEnabled);
        }

        if (hasSlice) {
            int slice = request->getParam("componentsPerCycle", true)->value().toInt();
            if (slice < 1 || slice > SystemConfig::MAX_COMPONENTS_PER_CYCLE_LIMIT) {
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"componentsPerCycle hors bornes (1-64)\"}");
                return;
            }
            SystemConfig::setComponentsPerCycle((uint8_t)slice);
        }
        
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
