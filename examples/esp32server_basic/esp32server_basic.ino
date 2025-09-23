#include <Esp32Server.h>

Esp32Server srv;
Esp32Server& esp32Server = srv; // Pour l'accès depuis esp32server_api.cpp

void setup(){
	Serial.begin(115200);
	while(!Serial){ delay(10); }
	const char* apSsid = "ESP32-Server";
	const char* apPass = "esp32pass";
	const char* host = "esp32server"; // accès via http://esp32server.local/
	srv.begin(apSsid, apPass, host);
	Serial.print("AP SSID: "); Serial.println(apSsid);
	Serial.print("AP PASS: "); Serial.println(apPass);
	Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
	Serial.print("mDNS: http://"); Serial.print(host); Serial.println(".local/");

	// Configurer le réseau STA si souhaité
	const char* staSsid = "manca";
	const char* staPass = "manca2022";
	// Optionnel: IP statique
	// srv.setStaticStaIp(IPAddress(192,168,1,50), IPAddress(192,168,1,1), IPAddress(255,255,255,0));
	srv.connectSta(staSsid, staPass);
}

void loop(){
	static uint32_t last=0; uint32_t now=millis();
	if(now-last>3000){
		last=now;
		Serial.print("AP IP: "); Serial.print(WiFi.softAPIP());
		Serial.print("  STA IP: "); Serial.println(WiFi.localIP());
	}
}
