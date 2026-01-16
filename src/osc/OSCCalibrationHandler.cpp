#include "OSCCalibrationHandler.h"
#include "../managers/ComponentManager.h"
#include "../hardware/MuxConstants.h"

void OSCCalibrationHandler::handleMessage(
    ComponentManager& manager,
    const String& address,
    float value,
    const String& arg_string
) {
    Serial.printf("[OSCCalibrationHandler] Callback OSC: address='%s', value=%f, arg_string='%s'\n", 
                 address.c_str(), value, arg_string.c_str());
    
    // Parser les commandes de calibrage : /mux/{id}/cal/{cmd} avec canal comme valeur
    if (!address.startsWith("/mux")) {
        Serial.printf("[OSCCalibrationHandler] Adresse ne commence pas par /mux, ignorée\n");
        return;
    }
    
    Serial.printf("[OSCCalibrationHandler] Adresse commence par /mux\n");
    
    // Format: /mux/{id}/cal/{cmd} avec canal passé comme valeur (int) dans le message OSC
    // Si value est absent ou invalide, traiter tous les canaux
    
    int mux_id_start = address.indexOf('/', 1); // Après "/mux"
    if (mux_id_start < 0) {
        Serial.printf("[OSCCalibrationHandler] ERREUR: pas de '/' après /mux\n");
        return;
    }
    
    // Extraire mux_id (peut être "0" ou "1")
    int mux_id_end = address.indexOf('/', mux_id_start + 1);
    if (mux_id_end < 0) {
        Serial.printf("[OSCCalibrationHandler] ERREUR: pas de '/' après mux_id\n");
        return;
    }
    
    String mux_id_str = address.substring(mux_id_start + 1, mux_id_end);
    uint8_t mux_id = mux_id_str.toInt();
    Serial.printf("[OSCCalibrationHandler] mux_id extrait: '%s' -> %d\n", mux_id_str.c_str(), mux_id);
    
    // Vérifier que mux_id est valide
    if (mux_id >= MAX_MUXES) {
        Serial.printf("[OSCCalibrationHandler] ERREUR: mux_id %d >= MAX_MUXES (%d)\n", mux_id, MAX_MUXES);
        return;
    }
    
    // Vérifier que c'est une commande cal (accepter /cal ou /calibrate)
    String after_id = address.substring(mux_id_end);
    Serial.printf("[OSCCalibrationHandler] Partie après mux_id: '%s', value=%f\n", 
                 after_id.c_str(), value);
    
    String cmd;
    bool all_channels = false;
    uint8_t channel = 0;
    
    // Extraire le canal depuis la valeur (value contient le numéro de canal 0-15)
    if (value >= 0 && value < 16 && (int)value == value) {
        channel = (uint8_t)value;
        all_channels = false;
    } else {
        // Pas de canal spécifié (ou invalide), traiter tous les canaux
        all_channels = true;
    }
    
    if (after_id == "/cal" || after_id == "/calibrate") {
        // Format: /mux/{id}/cal avec argument string dans le message OSC
        cmd = arg_string;
        Serial.printf("[OSCCalibrationHandler] Commande extraite de l'argument: '%s'\n", cmd.c_str());
    } else if (after_id.startsWith("/cal/") || after_id.startsWith("/calibrate/")) {
        // Format: /mux/{id}/cal/{cmd} avec canal comme valeur
        int cmd_start = after_id.startsWith("/cal/") ? 5 : 12; // "/cal/" = 5, "/calibrate/" = 12
        cmd = after_id.substring(cmd_start);
        Serial.printf("[OSCCalibrationHandler] Commande extraite de l'adresse: '%s'\n", cmd.c_str());
    } else {
        Serial.printf("[OSCCalibrationHandler] ERREUR: ne commence pas par /cal ou /calibrate\n");
        return;
    }
    
    // Parser selon le type de commande
    if (cmd == "min") {
        if (all_channels) {
            Serial.printf("[OSCCalibrationHandler] Commande: cal/min (tous les canaux)\n");
            manager.calibrateMux(mux_id, 0, true, true);
        } else {
            Serial.printf("[OSCCalibrationHandler] Commande: cal/min canal %d\n", channel);
            manager.calibrateMux(mux_id, channel, true, false);
        }
    } else if (cmd == "max") {
        if (all_channels) {
            Serial.printf("[OSCCalibrationHandler] Commande: cal/max (tous les canaux)\n");
            manager.calibrateMux(mux_id, 0, false, true);
        } else {
            Serial.printf("[OSCCalibrationHandler] Commande: cal/max canal %d\n", channel);
            manager.calibrateMux(mux_id, channel, false, false);
        }
    } else if (cmd == "reset") {
        if (all_channels) {
            Serial.printf("[OSCCalibrationHandler] Commande: cal/reset (tous les canaux)\n");
            manager.resetMuxThresholds(mux_id, 0, true);
        } else {
            Serial.printf("[OSCCalibrationHandler] Commande: cal/reset canal %d\n", channel);
            manager.resetMuxThresholds(mux_id, channel, false);
        }
    } else {
        Serial.printf("[OSCCalibrationHandler] ERREUR: commande inconnue: '%s'\n", cmd.c_str());
    }
}
