#pragma once

#include <Arduino.h>

// Forward declaration
class ComponentManager;

/**
 * @brief Handler OSC pour les commandes de calibrage MUX
 * 
 * Parse les messages OSC de type /mux/{id}/cal/{cmd} et
 * délègue au ComponentManager pour le calibrage.
 */
class OSCCalibrationHandler {
public:
    /**
     * @brief Traiter un message OSC de calibrage
     * @param manager Référence au ComponentManager
     * @param address Adresse OSC (ex: "/mux/0/cal/min")
     * @param value Valeur (peut contenir le numéro de canal 0-15)
     * @param arg_string Argument string (peut contenir la commande)
     */
    static void handleMessage(
        ComponentManager& manager,
        const String& address,
        float value,
        const String& arg_string
    );
};
