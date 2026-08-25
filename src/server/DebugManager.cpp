#include "DebugManager.h"
#include "WebDebugConsole.h"
#include <cstdarg>
#include <cstring>

// Instance globale (sera initialisée depuis le sketch)
DebugManager* g_debug = nullptr;

void DebugManager::debugNetwork(const char* format, ...) {
    if (!network) return;
    va_list args;
    va_start(args, format);
    printLog("[NETWORK] ", format, args);
    va_end(args);
}

void DebugManager::debugWebSocket(const char* format, ...) {
    if (!websocket) return;
    va_list args;
    va_start(args, format);
    printLog("[WEBSOCKET] ", format, args);
    va_end(args);
}

void DebugManager::debugAPI(const char* format, ...) {
    if (!api) return;
    va_list args;
    va_start(args, format);
    printLog("[API] ", format, args);
    va_end(args);
}

void DebugManager::debugCache(const char* format, ...) {
    if (!cache) return;
    va_list args;
    va_start(args, format);
    printLog("[CACHE] ", format, args);
    va_end(args);
}

void DebugManager::debugOSC(const char* format, ...) {
    if (!osc) return;
    va_list args;
    va_start(args, format);
    printLog("[OSC] ", format, args);
    va_end(args);
}

void DebugManager::debugMIDI(const char* format, ...) {
    if (!midi) return;
    va_list args;
    va_start(args, format);
    printLog("[MIDI] ", format, args);
    va_end(args);
}

void DebugManager::debugPins(const char* format, ...) {
    if (!pins) return;
    va_list args;
    va_start(args, format);
    printLog("[PINS] ", format, args);
    va_end(args);
}

void DebugManager::debugComponents(const char* format, ...) {
    if (!components) return;
    va_list args;
    va_start(args, format);
    printLog("[COMPONENTS] ", format, args);
    va_end(args);
}

void DebugManager::debugRtpMidi(const char* format, ...) {
    if (!rtpMidi) return;
    va_list args;
    va_start(args, format);
    printLog("[RTP-MIDI] ", format, args);
    va_end(args);
}

void DebugManager::error(const char* format, ...) {
    if (!shouldLog(ERROR)) return;
    va_list args;
    va_start(args, format);
    printLog("[ERROR] ", format, args);
    va_end(args);
}

void DebugManager::warning(const char* format, ...) {
    if (!shouldLog(WARNING)) return;
    va_list args;
    va_start(args, format);
    printLog("[WARNING] ", format, args);
    va_end(args);
}

void DebugManager::info(const char* format, ...) {
    if (!shouldLog(INFO)) return;
    va_list args;
    va_start(args, format);
    printLog("[INFO] ", format, args);
    va_end(args);
}

void DebugManager::debug(const char* format, ...) {
    if (!shouldLog(DEBUG)) return;
    va_list args;
    va_start(args, format);
    printLog("[DEBUG] ", format, args);
    va_end(args);
}

void DebugManager::printLog(const char* prefix, const char* format, va_list args) {
    char body[192];
    vsnprintf(body, sizeof(body), format, args);
    body[sizeof(body) - 1] = '\0';

    NIDMI_RAW_SERIAL.printf("[%lu] ", static_cast<unsigned long>(millis()));
    NIDMI_RAW_SERIAL.print(prefix);
    NIDMI_RAW_SERIAL.print(body);
    size_t len = strlen(body);
    if (len == 0 || body[len - 1] != '\n') {
        NIDMI_RAW_SERIAL.println();
    }

    char line[256];
    snprintf(line, sizeof(line), "[%lu] %s%s",
             static_cast<unsigned long>(millis()), prefix, body);
    len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        line[--len] = '\0';
    }
    nidmi_web_debug_append_line(line);
}

bool DebugManager::shouldLog(VerbosityLevel level) const {
    return level <= verbosity;
}
