#include "APICommon.h"
#include "../Globals.h"
#include "../server/ServerCore.h"
#include <Preferences.h>
#include <ESP.h>

void setupRTPAPI(AsyncWebServer& server) {
    // API - Configuration RTP-MIDI
    server.on("/api/rtp", HTTP_POST, [](AsyncWebServerRequest *request){
        if(request->hasParam("name", true) && request->hasParam("target", true)){
            String name = request->getParam("name", true)->value();
            String target = request->getParam("target", true)->value();
            
            // Sauvegarder en NVS
            Preferences preferences;
            preferences.begin("nidmi", false);
            preferences.putString("rtp_name", name);
            preferences.putString("rtp_target", target);
            preferences.end();
            
            // Redémarrer RTP-MIDI avec le nouveau nom
            serverCore.rtpMidi().stop();
            serverCore.rtpMidi().begin(name);
            
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            request->send(400, "application/json", "{\"error\":\"name and target required\"}");
        }
    });
    
    // API - Activation/Désactivation RTP-MIDI
    server.on("/api/rtp/enable", HTTP_POST, [](AsyncWebServerRequest *request){
        if(request->hasParam("enable", true)){
            String enabled = request->getParam("enable", true)->value();
            bool isEnabled = (enabled == "true");
            
            // Sauvegarder l'état en NVS
            Preferences preferences;
            preferences.begin("nidmi", false);
            preferences.putBool("rtp_enabled", isEnabled);
            preferences.end();
            
            if(isEnabled){
                // Récupérer le nom sauvegardé
                preferences.begin("nidmi", false);
                String name = preferences.getString("rtp_name", "ESP32-Studio");
                preferences.end();
                serverCore.rtpMidi().begin(name);
            } else {
                serverCore.rtpMidi().stop();
            }
            
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            request->send(400, "application/json", "{\"error\":\"enable parameter required\"}");
        }
    });
    
    // API - Statut RTP-MIDI
    server.on("/api/rtp/status", HTTP_GET, [](AsyncWebServerRequest *request){
        // Pour le sketch de test : enabled = true si RTP-MIDI est initialisé
        bool enabled = serverCore.rtpMidi().isInitialized();
        
        Preferences preferences;
        preferences.begin("nidmi", false);
        String name = preferences.getString("rtp_name", "ESP32-Test");
        String target = preferences.getString("rtp_target", "sta");
        preferences.end();
        
        String json = "{";
        json += "\"enabled\":" + String(enabled ? "true" : "false") + ",";
        json += "\"name\":\"" + name + "\",";
        json += "\"target\":\"" + target + "\",";
        json += "\"connected\":" + String(serverCore.rtpMidi().isConnected() ? "true" : "false");
        json += "}";
        request->send(200, "application/json", json);
    });
}
