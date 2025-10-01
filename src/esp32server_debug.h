#ifndef ESP32SERVER_DEBUG_H
#define ESP32SERVER_DEBUG_H

#include <Arduino.h>

// ============================================================================
// SYSTÈME DE DEBUG ULTRA-OPTIMISÉ - MÉTHODE 1
// ============================================================================
// 
// Version qui ne compile RIEN quand désactivé.
// Contrôle par macros de compilation.
//
// Usage dans votre sketch :
//   #define ESP32SERVER_DEBUG_OSC 1        // Activer debug OSC
//   #define ESP32SERVER_DEBUG_NETWORK 1   // Activer debug Network
//   // Pas de #define = module désactivé (zéro flash)
//
// ============================================================================

// Si aucun debug n'est défini, tout est désactivé (zéro overhead)
#ifndef ESP32SERVER_DEBUG_OSC
#define ESP32SERVER_DEBUG_OSC 0
#endif

#ifndef ESP32SERVER_DEBUG_NETWORK
#define ESP32SERVER_DEBUG_NETWORK 0
#endif

#ifndef ESP32SERVER_DEBUG_COMPONENTS
#define ESP32SERVER_DEBUG_COMPONENTS 0
#endif

#ifndef ESP32SERVER_DEBUG_WEBSOCKET
#define ESP32SERVER_DEBUG_WEBSOCKET 0
#endif

#ifndef ESP32SERVER_DEBUG_PINS
#define ESP32SERVER_DEBUG_PINS 0
#endif

#ifndef ESP32SERVER_DEBUG_CACHE
#define ESP32SERVER_DEBUG_CACHE 0
#endif

#ifndef ESP32SERVER_DEBUG_RTPMIDI
#define ESP32SERVER_DEBUG_RTPMIDI 0
#endif

#ifndef ESP32SERVER_DEBUG_API
#define ESP32SERVER_DEBUG_API 0
#endif

// Macros optimisées - zéro overhead quand désactivé
#if ESP32SERVER_DEBUG_OSC
#define debug_osc(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#else
#define debug_osc(fmt, ...) do {} while(0)
#endif

#if ESP32SERVER_DEBUG_NETWORK
#define debug_network(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#else
#define debug_network(fmt, ...) do {} while(0)
#endif

#if ESP32SERVER_DEBUG_COMPONENTS
#define debug_components(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#else
#define debug_components(fmt, ...) do {} while(0)
#endif

#if ESP32SERVER_DEBUG_WEBSOCKET
#define debug_websocket(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#else
#define debug_websocket(fmt, ...) do {} while(0)
#endif

#if ESP32SERVER_DEBUG_PINS
#define debug_pins(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#else
#define debug_pins(fmt, ...) do {} while(0)
#endif

#if ESP32SERVER_DEBUG_CACHE
#define debug_cache(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#else
#define debug_cache(fmt, ...) do {} while(0)
#endif

#if ESP32SERVER_DEBUG_RTPMIDI
#define debug_rtpmidi(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#else
#define debug_rtpmidi(fmt, ...) do {} while(0)
#endif

#if ESP32SERVER_DEBUG_API
#define debug_api(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#else
#define debug_api(fmt, ...) do {} while(0)
#endif

#endif // ESP32SERVER_DEBUG_H
