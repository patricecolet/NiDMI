#include "APICommon.h"
#include "../Globals.h"
#include "../server/ServerCore.h"
#include "../server/ServerCallbacks.h"
#include <Preferences.h>
#include <WiFi.h>

void setupNetworkAPI(AsyncWebServer& server) {
    // API - Statut général
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
        // Récupérer mDNS
        Preferences preferences;
        preferences.begin("nidmi", true);
        String mdnsName = preferences.getString("mdns_name", "nidmi");
        
        // Récupérer OSC
        String oscTarget = preferences.getString("osc_target", "sta");
        int oscPort = preferences.getInt("osc_port", 8000);
        String oscIp = preferences.getString("osc_ip", "");
        bool oscBroadcast = preferences.getBool("osc_broadcast", false);
        preferences.end();
        
        String json = "{";
        json += "\"ap_ssid\":\"" + WiFi.softAPSSID() + "\",";
        json += "\"ap_ip\":\"" + WiFi.softAPIP().toString() + "\",";
        json += "\"sta_ssid\":\"" + WiFi.SSID() + "\",";
        json += "\"sta_ip\":\"" + WiFi.localIP().toString() + "\",";
        json += "\"sta_connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
        json += "\"mdns_name\":\"" + mdnsName + "\",";
        json += "\"mdns_address\":\"" + mdnsName + ".local\",";
        json += "\"osc_target\":\"" + oscTarget + "\",";
        json += "\"osc_port\":" + String(oscPort);
        if(oscIp.length() > 0) {
            json += ",\"osc_ip\":\"" + oscIp + "\"";
        }
        json += ",\"osc_broadcast\":" + String(oscBroadcast ? "true" : "false");
        json += "}";
        request->send(200, "application/json", json);
    });
    
    // API - Configuration Wi-Fi STA
    server.on("/api/sta", HTTP_POST, [](AsyncWebServerRequest *request){
        if(request->hasParam("ssid", true) && request->hasParam("pass", true)){
            String ssid = request->getParam("ssid", true)->value();
            String pass = request->getParam("pass", true)->value();
            String ip = request->hasParam("ip", true) ? request->getParam("ip", true)->value() : String("");
            String gateway = request->hasParam("gw", true) ? request->getParam("gw", true)->value() : String("");
            String subnet = request->hasParam("sn", true) ? request->getParam("sn", true)->value() : String("");
            
            Preferences preferences;
            preferences.begin("nidmi", false);
            preferences.putString("sta_ssid", ssid);
            preferences.putString("sta_pass", pass);
            if(ip.length() > 0 && gateway.length() > 0 && subnet.length() > 0){
                preferences.putString("sta_ip", ip);
                preferences.putString("sta_gw", gateway);
                preferences.putString("sta_sn", subnet);
            }
            preferences.end();
            
            request->send(200, "application/json", "{\"status\":\"ok\",\"reboot\":true}");
            nidmi_requestReboot();
        } else {
            request->send(400, "application/json", "{\"error\":\"ssid and pass required\"}");
        }
    });

    // API - Lecture des identifiants STA stockés en NVS
    server.on("/api/sta/status", HTTP_GET, [](AsyncWebServerRequest *request){
        try {
            Preferences preferences;
            preferences.begin("nidmi", true);
            String ssid = preferences.getString("sta_ssid", "");
            String pass = preferences.getString("sta_pass", "");
            String ip   = preferences.getString("sta_ip",  "");
            String gw   = preferences.getString("sta_gw",  "");
            String sn   = preferences.getString("sta_sn",  "");
            preferences.end();
            
            String json = "{";
            json += "\"ssid\":\"" + ssid + "\",";
            json += "\"has_pass\":" + String(pass.length()>0 ? "true" : "false") + ",";
            json += "\"ip\":\"" + ip + "\",";
            json += "\"gw\":\"" + gw + "\",";
            json += "\"sn\":\"" + sn + "\"";
            json += "}";
            request->send(200, "application/json", json);
        } catch (...) {
            request->send(500, "application/json", "{\"error\":\"NVS read failed\"}");
        }
    });
    
    // API - Configuration mDNS
    server.on("/api/mdns", HTTP_POST, [](AsyncWebServerRequest *request){
        if(request->hasParam("name", true)){
            String name = request->getParam("name", true)->value();
            
            Preferences preferences;
            preferences.begin("nidmi", false);
            preferences.putString("mdns_name", name);
            preferences.end();
            
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            request->send(400, "application/json", "{\"error\":\"name required\"}");
        }
    });
    
    // API - Statut mDNS
    server.on("/api/mdns/status", HTTP_GET, [](AsyncWebServerRequest *request){
        Preferences preferences;
        preferences.begin("nidmi", false);
        String name = preferences.getString("mdns_name", "nidmi");
        preferences.end();
        String json = "{";
        json += "\"name\":\"" + name + "\"";
        json += "}";
        request->send(200, "application/json", json);
    });
}
