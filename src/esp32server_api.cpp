#include "Esp32Server.h"
#include "ui_index.h"
#include "pincaps_c3.h"
#include <Preferences.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>

Preferences preferences;

// Signale au runtime de recharger les configs pins
extern "C" void esp32server_requestReloadPins();

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.println("WebSocket client connected");
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.println("WebSocket client disconnected");
    }
}

// Fonction pour envoyer le statut RTP-MIDI via WebSocket
void sendRtpStatus(AsyncWebSocket& ws) {
    preferences.begin("esp32server", false);
    bool enabled = preferences.getBool("rtp_enabled", false);
    String name = preferences.getString("rtp_name", "ESP32-Studio");
    String target = preferences.getString("rtp_target", "sta");
    preferences.end();
    
    extern Esp32Server esp32Server;
    bool connected = esp32Server.rtpMidi().isConnected();
    
    String json = "{";
    json += "\"type\":\"rtp_status\",";
    json += "\"enabled\":" + String(enabled ? "true" : "false") + ",";
    json += "\"name\":\"" + name + "\",";
    json += "\"target\":\"" + target + "\",";
    json += "\"connected\":" + String(connected ? "true" : "false");
    json += "}";
    
    ws.textAll(json);
    Serial.println("RTP-MIDI status sent via WebSocket: " + json);
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
			String ip = request->hasParam("ip", true) ? request->getParam("ip", true)->value() : String("");
			String gateway = request->hasParam("gw", true) ? request->getParam("gw", true)->value() : String("");
			String subnet = request->hasParam("sn", true) ? request->getParam("sn", true)->value() : String("");
			
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

			// Vérification immédiate (lecture retour NVS)
			preferences.begin("esp32server", true);
			String chkSsid = preferences.getString("sta_ssid", "");
			String chkPass = preferences.getString("sta_pass", "");
			String chkIp   = preferences.getString("sta_ip",  "");
			String chkGw   = preferences.getString("sta_gw",  "");
			String chkSn   = preferences.getString("sta_sn",  "");
			preferences.end();
			Serial.println("[API/STA] Saved to NVS:");
			Serial.print("  ssid=\""); Serial.print(chkSsid); Serial.println("\"");
			Serial.print("  pass len="); Serial.println(chkPass.length());
			if(chkIp.length()>0){
				Serial.print("  ip="); Serial.print(chkIp);
				Serial.print(" gw="); Serial.print(chkGw);
				Serial.print(" sn="); Serial.println(chkSn);
			}
			
			// Connecter au Wi-Fi
			extern Esp32Server esp32Server;
			esp32Server.connectSta(ssid.c_str(), pass.c_str());
			
			request->send(200, "application/json", "{\"status\":\"ok\"}");
		} else {
			request->send(400, "application/json", "{\"error\":\"ssid and pass required\"}");
		}
	});

	// API - Lecture des identifiants STA stockés en NVS
	server.on("/api/sta/status", HTTP_GET, [](AsyncWebServerRequest *request){
		try {
			preferences.begin("esp32server", true);
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
			esp32Server.rtpMidi().begin(name); // Le nom sera lu depuis les préférences
			
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
		
		// Pour le sketch de test : enabled = true si RTP-MIDI est initialisé
		bool enabled = esp32Server.rtpMidi().isInitialized();
		
		preferences.begin("esp32server", false);
		String name = preferences.getString("rtp_name", "ESP32-Test");
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
	
	// API - Configuration mDNS
	server.on("/api/mdns", HTTP_POST, [](AsyncWebServerRequest *request){
		if(request->hasParam("name", true)){
			String name = request->getParam("name", true)->value();
			
			Serial.println("mDNS config - name: " + name);
			
			// Sauvegarder en NVS
			preferences.begin("esp32server", false);
			preferences.putString("mdns_name", name);
			preferences.end();
			
			Serial.println("Nom mDNS sauvegardé: " + name);
			Serial.println("Redémarrage nécessaire pour appliquer le nouveau nom");
			
			request->send(200, "application/json", "{\"status\":\"ok\"}");
		} else {
			request->send(400, "application/json", "{\"error\":\"name required\"}");
		}
	});
	
	// API - Statut mDNS
	server.on("/api/mdns/status", HTTP_GET, [](AsyncWebServerRequest *request){
		preferences.begin("esp32server", false);
		String name = preferences.getString("mdns_name", "esp32rtpmidi");
		preferences.end();
		String json = "{";
		json += "\"name\":\"" + name + "\"";
		json += "}";
		request->send(200, "application/json", json);
	});

	// API - Capacités des pins (C3)
	server.on("/api/pins/caps", HTTP_GET, [](AsyncWebServerRequest *request){
		String json = buildC3PinCapsJson();
		request->send(200, "application/json", json);
	});

	// API - Enregistrer la configuration d'un pin
	server.on("/api/pins/set", HTTP_POST, [](AsyncWebServerRequest *request){
		// Champs obligatoires
		if(!request->hasParam("pinLabel", true) || !request->hasParam("role", true)){
			request->send(400, "application/json", "{\"error\":\"pinLabel and role required\"}");
			return;
		}
		String pinLabel = request->getParam("pinLabel", true)->value();
		String role     = request->getParam("role", true)->value();

		// Champs optionnels connus (whitelist)
		auto getOpt = [&](const char* name){ return request->hasParam(name, true) ? request->getParam(name, true)->value() : String(""); };
		String rtpEnabled = getOpt("rtpEnabled");
		String rtpType    = getOpt("rtpType");
		String rtpNote    = getOpt("rtpNote");
		String rtpCc      = getOpt("rtpCc");
		String rtpPc      = getOpt("rtpPc");
		String rtpChan    = getOpt("rtpChan");
		String rtpCcOn    = getOpt("rtpCcOn");
		String rtpCcOff   = getOpt("rtpCcOff");
		String rtpVel     = getOpt("rtpVel");
		String ledMode    = getOpt("ledMode");

		// Construire un JSON compact à stocker
		String json = "{";
		json += "\"pinLabel\":\"" + pinLabel + "\",";
		json += "\"role\":\"" + role + "\"";
		if(rtpEnabled.length()) json += ",\"rtpEnabled\":" + String((rtpEnabled=="true")?"true":"false");
		if(rtpType.length())    json += ",\"rtpType\":\"" + rtpType + "\"";
		if(rtpNote.length())    json += ",\"rtpNote\":" + rtpNote;
		if(rtpCc.length())      json += ",\"rtpCc\":" + rtpCc;
		if(rtpPc.length())      json += ",\"rtpPc\":" + rtpPc;
		if(rtpChan.length())    json += ",\"rtpChan\":" + rtpChan;
		if(rtpCcOn.length())    json += ",\"rtpCcOn\":" + rtpCcOn;
		if(rtpCcOff.length())   json += ",\"rtpCcOff\":" + rtpCcOff;
		if(rtpVel.length())     json += ",\"rtpVel\":" + rtpVel;
		if(ledMode.length())    json += ",\"ledMode\":\"" + ledMode + "\"";
		json += "}";

		// Stocker en NVS sous une clé par pin
		String key = String("pin_") + pinLabel;
		preferences.begin("esp32server", false);
		bool ok = preferences.putString(key.c_str(), json) > 0;
		preferences.end();

		Serial.print("[API/PINS] Saved "); Serial.print(key); Serial.print(" = "); Serial.println(json);
		if(ok) {
			esp32server_requestReloadPins();
			request->send(200, "application/json", "{\"status\":\"ok\"}");
		}
		else   request->send(500, "application/json", "{\"error\":\"store failed\"}");
	});
	
	// WebSocket
	ws.onEvent(onWsEvent);
	server.addHandler(&ws);
}