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
	MDNS.begin(hostname);
	MDNS.addService("http", "tcp", 80);
	setupHttp(server, ws);
	server.begin();
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