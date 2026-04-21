#include "WebDebugConsole.h"

#if NIDMI_WEB_DEBUG_CONSOLE

#include <AsyncWebSocket.h>
#include <cstdarg>
#include <cstring>

static AsyncWebSocket* g_ws = nullptr;
static bool g_subscribe = false;

enum : uint16_t {
    kRingLines = 48,
    kLineCap = 200,
};

static char g_ring[kRingLines][kLineCap];
static uint16_t g_start = 0;
static uint16_t g_size = 0;

static void ring_push(const char* line) {
    uint16_t pos;
    if (g_size < kRingLines) {
        pos = (g_start + g_size) % kRingLines;
        ++g_size;
    } else {
        pos = g_start;
        g_start = (g_start + 1) % kRingLines;
    }
    strncpy(g_ring[pos], line, kLineCap - 1);
    g_ring[pos][kLineCap - 1] = '\0';
}

static void send_debug_log(AsyncWebSocketClient* client, const char* line) {
    if (!line) {
        return;
    }
    char msg[kLineCap + 16];
    snprintf(msg, sizeof(msg), "DEBUG_LOG:%s", line);
    if (client) {
        client->text(msg);
    }
}

static void flush_history_to(AsyncWebSocketClient* client) {
    if (!client) {
        return;
    }
    for (uint16_t i = 0; i < g_size; ++i) {
        uint16_t idx = (g_start + i) % kRingLines;
        send_debug_log(client, g_ring[idx]);
    }
}

void nidmi_web_debug_init(AsyncWebSocket* ws) {
    g_ws = ws;
}

bool nidmi_web_debug_is_supported() {
    return true;
}

void nidmi_web_debug_handle_ws_text(AsyncWebSocketClient* client, const String& message) {
    if (message == "DEBUG_CONSOLE:1") {
        g_subscribe = true;
        flush_history_to(client);
        if (client) {
            client->text("DEBUG_CONSOLE_STATE:1");
        }
    } else if (message == "DEBUG_CONSOLE:0") {
        g_subscribe = false;
        if (client) {
            client->text("DEBUG_CONSOLE_STATE:0");
        }
    }
}

void nidmi_web_debug_append_line(const char* line) {
    if (!line || !line[0]) {
        return;
    }
    ring_push(line);
    if (!g_subscribe || !g_ws) {
        return;
    }
    char msg[kLineCap + 16];
    snprintf(msg, sizeof(msg), "DEBUG_LOG:%s", line);
    g_ws->textAll(msg);
}

void nidmi_web_debug_log(const char* fmt, ...) {
    char line[kLineCap];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    line[sizeof(line) - 1] = '\0';

    Serial.println(line);
    nidmi_web_debug_append_line(line);
}

#endif
