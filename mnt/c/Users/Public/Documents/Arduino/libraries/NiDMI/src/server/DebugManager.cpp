#include "DebugManager.h"
#include <cstdarg>

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
    // Timestamp
    Serial.printf("[%lu] ", millis());
    
    // Prefix
    Serial.print(prefix);
    
    // Message formaté
    Serial.printf(format, args);
    
    // Nouvelle ligne si pas déjà présente
    size_t len = strlen(format);
    if (len == 0 || format[len-1] != '\n') {
        Serial.println();
    }
}

bool DebugManager::shouldLog(VerbosityLevel level) const {
    return level <= verbosity;
}
