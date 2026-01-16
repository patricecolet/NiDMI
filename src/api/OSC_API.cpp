#include "APICommon.h"
#include "../server/ServerCallbacks.h"/* Pour nidmi_requestReloadOsc */

void setupOSC_API(AsyncWebServer& server) {
    /* API - Configuration OSC */
    server.on("/api/osc", HTTP_POST, [](AsyncWebServerRequest *request){
        if(request->hasParam("target", true) && request->hasParam("port", true)){
            String target = request->getParam("target", true)->value();
            int port = request->getParam("port", true)->value().toInt();
            bool broadcast = request->hasParam("broadcast", true) && 
                           request->getParam("broadcast", true)->value() == "true";
            String interface = request->hasParam("interface", true) ? 
                             request->getParam("interface", true)->value() : "ap";
            
            /* Sauvegarder en NVS */
            Preferences preferences;
            preferences.begin("nidmi", false);
            preferences.putString("osc_target", target);
            preferences.putInt("osc_port", port);
            preferences.putBool("osc_broadcast", broadcast);
            preferences.putString("osc_interface", interface);
            preferences.end();
            
            /* Demander le rechargement OSC */
            // TODO: Implémenter nidmi_requestReloadOsc() si nécessaire
            // nidmi_requestReloadOsc();
            
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            request->send(400, "application/json", "{\"error\":\"target and port required\"}");
        }
    });

    /* API - Statut OSC */
    server.on("/api/osc/status", HTTP_GET, [](AsyncWebServerRequest *request){
        Preferences preferences;
        preferences.begin("nidmi", true);
        String target = preferences.getString("osc_target", "192.168.4.100");
        int port = preferences.getInt("osc_port", 8000);
        bool broadcast = preferences.getBool("osc_broadcast", false);
        String interface = preferences.getString("osc_interface", "ap");
        preferences.end();
        String json = "{";
        json += "\"target\":\"" + target + "\",";
        json += "\"port\":" + String(port) + ",";
        json += "\"broadcast\":" + String(broadcast ? "true" : "false") + ",";
        json += "\"interface\":\"" + interface + "\"";
        json += "}";
        request->send(200, "application/json", json);
    });
}
