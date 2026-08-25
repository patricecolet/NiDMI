#include "WebDebugConsole.h"
#include <cstdarg>
#include <cstdio>

#if NIDMI_WEB_DEBUG_CONSOLE

#include <AsyncWebSocket.h>
#include <cstdarg>
#include <cstring>

static AsyncWebSocket* g_ws = nullptr;
static bool g_subscribe = false;

enum : uint16_t {
#if NIDMI_USB_NET
    /* Variant --usb-net : la console web est la seule sortie de logs, il faut
       de quoi garder tout le journal de boot. */
    kRingLines = 96,
#else
    kRingLines = 48,
#endif
    kLineCap = 200,
};

static char g_ring[kRingLines][kLineCap];
static uint16_t g_start = 0;
static uint16_t g_size = 0;

/* Lignes poussées depuis le boot, et lignes déjà émises sur le WebSocket. La
   différence est ce que la pompe doit envoyer ; compteurs 32 bits pour ne pas
   dépendre de la position dans le ring. */
static uint32_t g_pushed = 0;
static uint32_t g_sent = 0;

/* Le ring est alimenté depuis n'importe quelle tâche (traitement composants sur
   le cœur 0, event WiFi, tâche serveur), d'où le verrou. */
static portMUX_TYPE g_mux = portMUX_INITIALIZER_UNLOCKED;

static constexpr size_t kChunkCap = 1024;

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
    ++g_pushed;
}

static void flush_history_to(AsyncWebSocketClient* client) {
    if (!client) {
        return;
    }
    /* Groupé en paquets : le client concatène le payload tel quel, et envoyer
       tout le ring ligne par ligne dépasse la file d'envoi du WebSocket
       (WS_MAX_QUEUED_MESSAGES) — l'historique arriverait tronqué. */
    String chunk;
    chunk.reserve(kChunkCap + kLineCap);
    char line[kLineCap];
    for (uint16_t i = 0;; ++i) {
        /* Copie sous verrou : une autre tâche peut réécrire la case pendant la
           lecture (le ring tourne). */
        portENTER_CRITICAL(&g_mux);
        const bool done = (i >= g_size);
        if (!done) {
            memcpy(line, g_ring[(g_start + i) % kRingLines], kLineCap);
        }
        portEXIT_CRITICAL(&g_mux);
        if (done) {
            break;
        }
        if (chunk.length() + strlen(line) + 1 > kChunkCap) {
            client->text("DEBUG_LOG:" + chunk);
            chunk = "";
        }
        if (chunk.length() > 0) {
            chunk += '\n';
        }
        chunk += line;
    }
    if (chunk.length() > 0) {
        client->text("DEBUG_LOG:" + chunk);
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
        portENTER_CRITICAL(&g_mux);
        g_sent = g_pushed;
        portEXIT_CRITICAL(&g_mux);
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
    /* Écriture dans le ring uniquement : l'appelant peut être n'importe quelle
       tâche (event WiFi, cœur 0), et toucher AsyncWebSocket hors de la tâche
       Arduino provoque des races avec la tâche serveur. L'émission est faite
       par nidmi_web_debug_pump(), appelée depuis la boucle principale. */
    portENTER_CRITICAL(&g_mux);
    ring_push(line);
    portEXIT_CRITICAL(&g_mux);
}

void nidmi_web_debug_pump() {
    if (!g_subscribe || !g_ws) {
        /* Personne n'écoute : ne pas accumuler d'arriéré à déverser au moment
           où quelqu'un s'abonne (l'historique est envoyé séparément). */
        portENTER_CRITICAL(&g_mux);
        g_sent = g_pushed;
        portEXIT_CRITICAL(&g_mux);
        return;
    }

    String chunk;
    chunk.reserve(kChunkCap + kLineCap);
    char line[kLineCap];

    for (uint16_t guard = 0; guard < kRingLines; ++guard) {
        portENTER_CRITICAL(&g_mux);
        if (g_sent >= g_pushed) {
            portEXIT_CRITICAL(&g_mux);
            break;
        }
        /* Lignes écrasées entre-temps : reprendre à la plus vieille encore
           présente plutôt que de lire une case déjà réécrite. */
        if (g_pushed - g_sent > g_size) {
            g_sent = g_pushed - g_size;
        }
        const uint16_t offset = static_cast<uint16_t>(g_size - (g_pushed - g_sent));
        memcpy(line, g_ring[(g_start + offset) % kRingLines], kLineCap);
        ++g_sent;
        portEXIT_CRITICAL(&g_mux);

        if (chunk.length() + strlen(line) + 1 > kChunkCap) {
            g_ws->textAll("DEBUG_LOG:" + chunk);
            chunk = "";
        }
        if (chunk.length() > 0) {
            chunk += '\n';
        }
        chunk += line;
    }

    if (chunk.length() > 0) {
        g_ws->textAll("DEBUG_LOG:" + chunk);
    }
}

#endif

// Log "tee" : toujours compilé (Serial sur toutes les cartes, + console si S3).
void nidmi_web_debug_log(const char* fmt, ...) {
    char line[200];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    line[sizeof(line) - 1] = '\0';

    NIDMI_RAW_SERIAL.println(line);
#if NIDMI_WEB_DEBUG_CONSOLE
    nidmi_web_debug_append_line(line);
#endif
}
