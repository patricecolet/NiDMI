#pragma once

#include <Arduino.h>

/* Écriture sur le port série réel. Redéfini par src/utils/SerialTee.h dans le
   variant --usb-net, où Serial est détourné vers la console web : les appelants
   ci-dessous poussent déjà la ligne dans le ring eux-mêmes. */
#ifndef NIDMI_RAW_SERIAL
#define NIDMI_RAW_SERIAL Serial
#endif

#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ARDUINO_ESP32S3_DEV) || defined(ARDUINO_ESP32S3)
#define NIDMI_WEB_DEBUG_CONSOLE 1
#else
#define NIDMI_WEB_DEBUG_CONSOLE 0
#endif

class AsyncWebSocket;
class AsyncWebSocketClient;

// Log "tee" : écrit TOUJOURS sur Serial, et en plus dans la console web si
// supportée (S3). Compilé sur toutes les cartes (sur C3 = Serial seul).
void nidmi_web_debug_log(const char* fmt, ...);

#if NIDMI_WEB_DEBUG_CONSOLE

void nidmi_web_debug_init(AsyncWebSocket* ws);
void nidmi_web_debug_handle_ws_text(AsyncWebSocketClient* client, const String& message);
void nidmi_web_debug_append_line(const char* line);
/* À appeler depuis la boucle principale : c'est le seul endroit qui pousse sur
   le WebSocket, les producteurs de logs n'écrivent que dans le ring. */
void nidmi_web_debug_pump();
bool nidmi_web_debug_is_supported();

#else

inline void nidmi_web_debug_init(AsyncWebSocket*) {}
inline void nidmi_web_debug_handle_ws_text(AsyncWebSocketClient*, const String&) {}
inline void nidmi_web_debug_append_line(const char*) {}
inline void nidmi_web_debug_pump() {}
inline bool nidmi_web_debug_is_supported() { return false; }

#endif

// Disponible sur toutes les cartes : Serial + console web (si S3).
#define NIDMI_WEB_LOG(...) nidmi_web_debug_log(__VA_ARGS__)
