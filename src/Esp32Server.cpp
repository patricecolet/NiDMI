#include "Esp32Server.h"
#include <Preferences.h>

Esp32Server esp32Server;            // noyau serveur (web, rtp)
Esp32ServerAPI esp32server;         // façade style midimap

// Demande de rechargement des configs pins depuis l'API
static bool g_requestReloadPins = false;
extern "C" void esp32server_requestReloadPins(){ g_requestReloadPins = true; }

void esp32server_begin() {
    // Logs
    Serial.begin(115200);
    delay(50);

    // Lire nom serveur + STA depuis NVS
    Preferences preferences;
    preferences.begin("esp32server", false);
    String serverName = preferences.getString("mdns_name", "esp32rtpmidi");
    String staSsid    = preferences.getString("sta_ssid", "");
    String staPass    = preferences.getString("sta_pass", "");
    String staIpStr   = preferences.getString("sta_ip",  "");
    String staGwStr   = preferences.getString("sta_gw",  "");
    String staSnStr   = preferences.getString("sta_sn",  "");
    preferences.end();
    
    // Debug: afficher ce qui est lu depuis NVS
    Serial.println("[ESP32Server] NVS Debug:");
    Serial.print("  mdns_name: \""); Serial.print(serverName); Serial.println("\"");
    Serial.print("  sta_ssid: \""); Serial.print(staSsid); Serial.println("\"");
    Serial.print("  sta_pass length: "); Serial.println(staPass.length());
    Serial.print("  sta_ip: \""); Serial.print(staIpStr); Serial.println("\"");
    Serial.print("  sta_gw: \""); Serial.print(staGwStr); Serial.println("\"");
    Serial.print("  sta_sn: \""); Serial.print(staSnStr); Serial.println("\"");

    const char* apSsid = serverName.c_str();
    const char* apPass = "esp32pass";
    const char* host   = serverName.c_str();

    // Démarre web + mDNS + AP
    esp32Server.begin(apSsid, apPass, host);

    // Tente STA si configurée
    if (staSsid.length() > 0) {
        if (staIpStr.length() > 0 && staGwStr.length() > 0 && staSnStr.length() > 0) {
            IPAddress ip, gw, sn;
            if (ip.fromString(staIpStr) && gw.fromString(staGwStr) && sn.fromString(staSnStr)) {
                esp32Server.setStaticStaIp(ip, gw, sn);
                Serial.print("[ESP32Server] STA static IP: "); Serial.print(staIpStr);
                Serial.print(" GW: "); Serial.print(staGwStr);
                Serial.print(" SN: "); Serial.println(staSnStr);
            }
        }
        esp32Server.connectSta(staSsid.c_str(), staPass.c_str());
        Serial.print("[ESP32Server] Connecting STA SSID: "); Serial.println(staSsid);
    } else {
        Serial.println("[ESP32Server] No STA credentials in NVS");
    }
}

void Esp32ServerAPI::begin() {
    esp32server_begin();
}

void Esp32ServerAPI::loop() {
    // Tâches périodiques minimales
    esp32Server.rtpMidi().update();

    // Initialiser RTP-MIDI automatiquement dès que la STA est connectée
    static bool rtpInitialized = false;
    if (!rtpInitialized && WiFi.status() == WL_CONNECTED) {
        Preferences preferences;
        preferences.begin("esp32server", true);
        String serverName = preferences.getString("mdns_name", "esp32rtpmidi");
        preferences.end();
        esp32Server.rtpMidi().begin(serverName);
        Serial.print("[ESP32Server] RTP-MIDI initialized (STA up) with name: ");
        Serial.println(serverName);
        rtpInitialized = true;
    }

    // Exécution basique des rôles Pins (démo)
    struct PinRuntimeCfg { bool valid=false; String role; bool rtpEnabled=false; String rtpType; int rtpNote=60; int rtpCc=7; int rtpChan=1; int gpio=-1; };
    static bool pinCfgLoaded = false;
    static PinRuntimeCfg pinCfg[11]; // D0..D10 indexés par numéro
    auto indexToGpio = [](int idx)->int{
        // Basé sur pincaps_c3.cpp
        switch(idx){
            case 0: return 2;  case 1: return 3;  case 2: return 4;  case 3: return 5;
            case 4: return 6;  case 5: return 7;  case 6: return 21; case 7: return 20;
            case 8: return 8;  case 9: return 9;  case 10: return 10; default: return -1;
        }
    };
    auto extractInt = [](const String& src, const char* key, int def)->int{
        String pat = String("\"") + key + "\":"; int p = src.indexOf(pat); if(p<0) return def; p += pat.length();
        while(p < (int)src.length() && (src[p]==' ')) p++;
        int end = p; while(end < (int)src.length() && isdigit(src[end])) end++;
        if(end>p) return src.substring(p,end).toInt();
        return def;
    };
    auto extractBool = [](const String& src, const char* key, bool def)->bool{
        String pat = String("\"") + key + "\":"; int p = src.indexOf(pat); if(p<0) return def; p += pat.length();
        while(p < (int)src.length() && (src[p]==' ')) p++;
        if(src.startsWith("true", p)) return true; if(src.startsWith("false", p)) return false; return def;
    };
    auto extractStr = [](const String& src, const char* key, String def)->String{
        String pat = String("\"") + key + "\":\""; int p = src.indexOf(pat); if(p<0) return def; p += pat.length();
        int end = src.indexOf('"', p); if(end<0) return def; return src.substring(p,end);
    };
    if(g_requestReloadPins){ pinCfgLoaded = false; g_requestReloadPins = false; }
    if(!pinCfgLoaded){
        Preferences preferences;
        preferences.begin("esp32server", true);
        for(int i=0;i<=10;i++){
            String key = String("pin_D") + String(i);
            String json = preferences.getString(key.c_str(), "");
            if(json.length()==0) continue;
            PinRuntimeCfg c; c.valid = true;
            c.role = extractStr(json, "role", "");
            c.rtpEnabled = extractBool(json, "rtpEnabled", false);
            c.rtpType = extractStr(json, "rtpType", "");
            c.rtpNote = extractInt(json, "rtpNote", 60);
            c.rtpCc   = extractInt(json, "rtpCc", 7);
            c.rtpChan = extractInt(json, "rtpChan", 1);
            c.gpio = indexToGpio(i);
            if(c.gpio>=0){ pinCfg[i] = c; Serial.printf("[Pins] Loaded D%d role=%s type=%s chan=%d\n", i, c.role.c_str(), c.rtpType.c_str(), c.rtpChan); }
        }
        preferences.end();
        // Prépare GPIO modes
        for(int i=0;i<=10;i++) if(pinCfg[i].valid){
            if(pinCfg[i].role == "Potentiomètre"){
                // ADC auto
            } else if(pinCfg[i].role == "Bouton"){
                pinMode(pinCfg[i].gpio, INPUT_PULLUP);
            } else if(pinCfg[i].role == "LED"){
                pinMode(pinCfg[i].gpio, OUTPUT);
                digitalWrite(pinCfg[i].gpio, LOW);
            }
        }
        pinCfgLoaded = true;
    }
    static int lastCcVal[11]; static bool lastBtnOn[11]; static uint32_t lastDebounceMs[11]; static bool debounceState[11]; static bool initStates=false;
    if(!initStates){ for(int i=0;i<=10;i++){ lastCcVal[i] = -1; lastBtnOn[i]=false; lastDebounceMs[i]=0; debounceState[i]=false; } initStates=true; }
    uint32_t nowMs = millis();
    for(int i=0;i<=10;i++){
        if(!pinCfg[i].valid || !pinCfg[i].rtpEnabled || pinCfg[i].gpio<0) continue;
        // Potentiomètre → CC
        if(pinCfg[i].role=="Potentiomètre" && (pinCfg[i].rtpType=="Control Change")){
            int raw = analogRead(pinCfg[i].gpio);
            int val = map(raw, 0, 4095, 0, 127);
            if(val<0) val=0; if(val>127) val=127;
            if(lastCcVal[i] < 0 || abs(val - lastCcVal[i]) >= 2){
                esp32Server.rtpMidi().sendControlChange((uint8_t)pinCfg[i].rtpChan, (uint8_t)pinCfg[i].rtpCc, (uint8_t)val);
                lastCcVal[i] = val;
            }
        }
        // Bouton → Note
        else if(pinCfg[i].role=="Bouton" && pinCfg[i].rtpType=="Note"){
            int readV = digitalRead(pinCfg[i].gpio); // INPUT_PULLUP: 0 = press
            bool pressed = (readV == LOW);
            if(pressed != debounceState[i]){ lastDebounceMs[i] = nowMs; debounceState[i] = pressed; }
            if(nowMs - lastDebounceMs[i] > 30){
                if(pressed && !lastBtnOn[i]){
                    esp32Server.rtpMidi().sendNoteOn((uint8_t)pinCfg[i].rtpChan, (uint8_t)pinCfg[i].rtpNote, 127);
                    lastBtnOn[i] = true;
                } else if(!pressed && lastBtnOn[i]){
                    esp32Server.rtpMidi().sendNoteOff((uint8_t)pinCfg[i].rtpChan, (uint8_t)pinCfg[i].rtpNote, 0);
                    lastBtnOn[i] = false;
                }
            }
        }
        // LED: à implémenter (suivi MIDI entrant)
    }
}

#include "Esp32Server.h"
#include <ESPmDNS.h>
#include <Preferences.h>

// Déclaration de la fonction setupHttp définie dans esp32server_api.cpp
void setupHttp(AsyncWebServer& server, AsyncWebSocket& ws);

Esp32Server::Esp32Server()
	: server(80), ws("/ws") {}

void Esp32Server::begin(const char* apSsid, const char* apPass, const char* hostname) {
	WiFi.mode(WIFI_MODE_APSTA);
	WiFi.softAP(apSsid, apPass);
	IPAddress apIp = WiFi.softAPIP();
	Serial.begin(115200);
	Serial.println();
	Serial.println("[ESP32Server] AP up");
	Serial.print("  SSID: "); Serial.println(apSsid);
	Serial.print("  PASS: "); Serial.println(apPass);
	Serial.print("  AP IP: "); Serial.println(apIp);

	bool mdnsOk = MDNS.begin(hostname);
	if (mdnsOk) {
		MDNS.addService("http", "tcp", 80);
		Serial.print("[ESP32Server] mDNS: http://"); Serial.print(hostname); Serial.println(".local/");
	} else {
		Serial.println("[ESP32Server] mDNS init failed");
	}
	setupHttp(server, ws);
	server.begin();
	Serial.println("[ESP32Server] HTTP server started on / (Async)");
}

void Esp32Server::connectSta(const char* staSsid, const char* staPass) {
	if (useStaticSta) {
		WiFi.config(staIp, staGw, staSn);
	}
	WiFi.begin(staSsid, staPass);
}

void Esp32Server::setStaticStaIp(IPAddress ip, IPAddress gateway, IPAddress subnet) {
	useStaticSta = true; staIp = ip; staGw = gateway; staSn = subnet;
}

AsyncWebServer& Esp32Server::web() { return server; }
AsyncWebSocket& Esp32Server::websocket() { return ws; }
RtpMidi& Esp32Server::rtpMidi() { return rtpMidiInstance; }