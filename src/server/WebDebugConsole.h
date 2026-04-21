#pragma once

#include <Arduino.h>

#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ARDUINO_ESP32S3_DEV) || defined(ARDUINO_ESP32S3)
#define NIDMI_WEB_DEBUG_CONSOLE 1
#else
#define NIDMI_WEB_DEBUG_CONSOLE 0
#endif

class AsyncWebSocket;
class AsyncWebSocketClient;

#if NIDMI_WEB_DEBUG_CONSOLE

void nidmi_web_debug_init(AsyncWebSocket* ws);
void nidmi_web_debug_handle_ws_text(AsyncWebSocketClient* client, const String& message);
void nidmi_web_debug_append_line(const char* line);
void nidmi_web_debug_log(const char* fmt, ...);
bool nidmi_web_debug_is_supported();

#else

inline void nidmi_web_debug_init(AsyncWebSocket*) {}
inline void nidmi_web_debug_handle_ws_text(AsyncWebSocketClient*, const String&) {}
inline void nidmi_web_debug_append_line(const char*) {}
inline bool nidmi_web_debug_is_supported() { return false; }

#endif

#if NIDMI_WEB_DEBUG_CONSOLE
#define NIDMI_WEB_LOG(...) nidmi_web_debug_log(__VA_ARGS__)
#else
#define NIDMI_WEB_LOG(...) ((void)0)
#endif
