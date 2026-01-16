#pragma once

#include <Arduino.h>

/**
 * @brief Interface publique NiDMI (style midimap)
 * 
 * Cette interface fournit une API simple style midimap :
 * - Objet global nidmi avec begin()/loop()
 * - Gestion automatique des composants
 * - Configuration via interface web
 * 
 * Usage :
 * ```cpp
 * #include <NiDMI.h>
 * 
 * void setup() {
 *     nidmi.begin();
 * }
 * 
 * void loop() {
 *     nidmi.loop();
 * }
 * ```
 */

// API style midimap: objet global avec begin()/loop()
struct NiDMIServer {
    void begin();
    void loop();
};

// Instance globale exposée
extern NiDMIServer nidmi;

// Fonction legacy (optionnelle): init unique
void nidmi_begin();
