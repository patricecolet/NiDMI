#ifndef ESP32SERVER_H
#define ESP32SERVER_H

#include <Arduino.h>

/**
 * @brief Interface publique ESP32Server (style midimap)
 * 
 * Cette interface fournit une API simple style midimap :
 * - Objet global esp32server avec begin()/loop()
 * - Gestion automatique des composants
 * - Configuration via interface web
 * 
 * Usage :
 * ```cpp
 * #include "Esp32Server.h"
 * 
 * void setup() {
 *     esp32server.begin();
 * }
 * 
 * void loop() {
 *     esp32server.loop();
 * }
 * ```
 */

// API style midimap: objet global avec begin()/loop()
struct Esp32Server {
    void begin();
    void loop();
};

// Instance globale exposée
extern Esp32Server esp32server;

// Fonction legacy (optionnelle): init unique
void esp32server_begin();

#endif // ESP32SERVER_H