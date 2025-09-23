#include "Esp32Server.h"
#include "ui_index.h"
#include "pincaps_c3.h"
#include <Preferences.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>

Preferences preferences;

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.println("WebSocket client connected");
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.println("WebSocket client disconnected");
    }
}

void setupHttp(AsyncWebServer& server, AsyncWebSocket& ws) {
	// Page principale
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(200, "text/html", INDEX_HTML);
	});
	
	// API - Statut système
	server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
		String json = "{";
		json += "\"ap_ssid\":\"" + WiFi.softAPSSID() + "\",";
		json += "\"ap_ip\":\"" + WiFi.softAPIP().toString() + "\",";
		json += "\"sta_ssid\":\"" + WiFi.SSID() + "\",";
		json += "\"sta_ip\":\"" + WiFi.localIP().toString() + "\",";
		json += "\"sta_connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false");
		json += "}";
		request->send(200, "application/json", json);
	});
	
	// API - Configuration Wi-Fi STA
	server.on("/api/sta", HTTP_POST, [](AsyncWebServerRequest *request){
		if(request->hasParam("ssid", true) && request->hasParam("pass", true)){
			String ssid = request->getParam("ssid", true)->value();
			String pass = request->getParam("pass", true)->value();
			String ip = request->getParam("ip", true)->value();
			String gateway = request->getParam("gw", true)->value();
			String subnet = request->getParam("sn", true)->value();
			
			// Sauvegarder en NVS
			preferences.begin("esp32server", false);
			preferences.putString("sta_ssid", ssid);
			preferences.putString("sta_pass", pass);
			if(ip.length() > 0 && gateway.length() > 0 && subnet.length() > 0){
				preferences.putString("sta_ip", ip);
				preferences.putString("sta_gw", gateway);
				preferences.putString("sta_sn", subnet);
			}
			preferences.end();
			
			// Connecter au Wi-Fi
			extern Esp32Server esp32Server;
			esp32Server.connectSta(ssid.c_str(), pass.c_str());
			
			request->send(200, "application/json", "{\"status\":\"ok\"}");
		} else {
			request->send(400, "application/json", "{\"error\":\"ssid and pass required\"}");
		}
	});
	
	// API - Configuration RTP-MIDI
	server.on("/api/rtp", HTTP_POST, [](AsyncWebServerRequest *request){
		if(request->hasParam("name", true) && request->hasParam("target", true)){
			String name = request->getParam("name", true)->value();
			String target = request->getParam("target", true)->value();
			
			Serial.println("RTP-MIDI config - name: " + name + ", target: " + target);
			
			// Sauvegarder en NVS
			preferences.begin("esp32server", false);
			preferences.putString("rtp_name", name);
			preferences.putString("rtp_target", target);
			preferences.end();
			
			// Redémarrer RTP-MIDI avec le nouveau nom
			extern Esp32Server esp32Server;
			esp32Server.rtpMidi().stop();
			esp32Server.rtpMidi().begin(name);
			
			request->send(200, "application/json", "{\"status\":\"ok\"}");
		} else {
			request->send(400, "application/json", "{\"error\":\"name and target required\"}");
		}
	});
	
	// API - Activation/Désactivation RTP-MIDI
	server.on("/api/rtp/enable", HTTP_POST, [](AsyncWebServerRequest *request){
		Serial.println("RTP-MIDI enable request received");
		if(request->hasParam("enable", true)){
			String enabled = request->getParam("enable", true)->value();
			Serial.println("Enable parameter: " + enabled);
			bool isEnabled = (enabled == "true");
			
			// Sauvegarder l'état en NVS
			preferences.begin("esp32server", false);
			preferences.putBool("rtp_enabled", isEnabled);
			preferences.end();
			
			extern Esp32Server esp32Server;
			if(isEnabled){
				// Récupérer le nom sauvegardé
				preferences.begin("esp32server", false);
				String name = preferences.getString("rtp_name", "ESP32-Studio");
				preferences.end();
				esp32Server.rtpMidi().begin(name);
				Serial.println("RTP-MIDI activé avec le nom: " + name);
			} else {
				esp32Server.rtpMidi().stop();
				Serial.println("RTP-MIDI désactivé");
			}
			
			request->send(200, "application/json", "{\"status\":\"ok\"}");
		} else {
			Serial.println("RTP-MIDI enable request missing 'enable' parameter");
			request->send(400, "application/json", "{\"error\":\"enable parameter required\"}");
		}
	});
	
	// API - Statut RTP-MIDI
	server.on("/api/rtp/status", HTTP_GET, [](AsyncWebServerRequest *request){
		extern Esp32Server esp32Server;
		preferences.begin("esp32server", false);
		bool enabled = preferences.getBool("rtp_enabled", false);
		String name = preferences.getString("rtp_name", "ESP32-Studio");
		String target = preferences.getString("rtp_target", "sta");
		preferences.end();
		
		String json = "{";
		json += "\"enabled\":" + String(enabled ? "true" : "false") + ",";
		json += "\"name\":\"" + name + "\",";
		json += "\"target\":\"" + target + "\",";
		json += "\"connected\":" + String(esp32Server.rtpMidi().isConnected() ? "true" : "false");
		json += "}";
		request->send(200, "application/json", json);
	});
	
	// API - Configuration OSC
	server.on("/api/osc", HTTP_POST, [](AsyncWebServerRequest *request){
		if(request->hasParam("target", true) && request->hasParam("port", true)){
			String target = request->getParam("target", true)->value();
			int port = request->getParam("port", true)->value().toInt();
			// Sauvegarder en NVS
			preferences.begin("esp32server", false);
			preferences.putString("osc_target", target);
			preferences.putInt("osc_port", port);
			preferences.end();
			request->send(200, "application/json", "{\"status\":\"ok\"}");
		} else {
			request->send(400, "application/json", "{\"error\":\"target and port required\"}");
		}
	});

	// API - Statut OSC
	server.on("/api/osc/status", HTTP_GET, [](AsyncWebServerRequest *request){
		preferences.begin("esp32server", false);
		String target = preferences.getString("osc_target", "sta");
		int port = preferences.getInt("osc_port", 8000);
		preferences.end();
		String json = "{";
		json += "\"target\":\"" + target + "\",";
		json += "\"port\":" + String(port);
		json += "}";
		request->send(200, "application/json", json);
	});

	// API - Capacités des pins (C3)
	server.on("/api/pins/caps", HTTP_GET, [](AsyncWebServerRequest *request){
		String json = buildC3PinCapsJson();
		request->send(200, "application/json", json);
	});
	
	// WebSocket
	ws.onEvent(onWsEvent);
	server.addHandler(&ws);
}