#include "APICommon.h"
#include "Esp32Server.h" /* Pour esp32server_requestReloadPins */
#include "../ui_index.h" /* Pour INDEX_HTML */  // ← AJOUTER CETTE LIGNE
void setupNetworkAPI(AsyncWebServer& server) {
    /* API - Statut général */
        /* Page principale - Servir le HTML embarqué */
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", INDEX_HTML);
    });
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = "{";
        json += "\"ap_ssid\":\"" + WiFi.softAPSSID() + "\",";
        json += "\"ap_ip\":\"" + WiFi.softAPIP().toString() + "\",";
        json += "\"sta_ssid\":\"" + WiFi.SSID() + "\",";
        json += "\"sta_ip\":\"" + WiFi.localIP().toString() + "\",";
        json += "\"sta_connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false\n");
        json += "}";
        request->send(200, "application/json", json);
    });

    /* API - Configuration mDNS */
    server.on("/api/mdns", HTTP_POST, [](AsyncWebServerRequest *request){
        if(request->hasParam("name", true)){
            String name = request->getParam("name", true)->value();
            
            /* Sauvegarder en NVS */
            Preferences preferences;
            preferences.begin("esp32server", false);
            preferences.putString("mdns_name", name);
            preferences.end();
            
            request->send(200, "application/json", "{\"status\":\"ok\"}\n");
        } else {
            request->send(400, "application/json", "{\"error\":\"name required\"}\n");
        }
    });

    /* API - Statut mDNS */
    server.on("/api/mdns/status", HTTP_GET, [](AsyncWebServerRequest *request){
        Preferences preferences;
        preferences.begin("esp32server", true);
        String name = preferences.getString("mdns_name", "esp32rtpmidi\n");
        preferences.end();
        String json = "{";
        json += "\"name\":\"" + name + "\"";
        json += "}";
        request->send(200, "application/json", json);
    });

    /* API - Configuration WiFi Station */
    server.on("/api/sta", HTTP_POST, [](AsyncWebServerRequest *request){
        if(request->hasParam("ssid", true)){
            String ssid = request->getParam("ssid", true)->value();
            String pass = request->hasParam("pass", true) ? request->getParam("pass", true)->value() : "";
            
            /* Sauvegarder en NVS */
            Preferences preferences;
            preferences.begin("esp32server", false);
            preferences.putString("sta_ssid", ssid);
            preferences.putString("sta_pass", pass);
            preferences.end();
            
            request->send(200, "application/json", "{\"status\":\"ok\"}\n");
        } else {
            request->send(400, "application/json", "{\"error\":\"ssid required\"}\n");
        }
    });

    /* API - Statut RTP-MIDI */
    server.on("/api/rtpmidi/status", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = "{";
        json += "\"connected\":" + String("false"); /* TODO: Intégrer ServerCore */
        json += "}";
        request->send(200, "application/json", json);
    });
}
