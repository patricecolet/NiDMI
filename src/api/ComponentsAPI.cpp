#include "APICommon.h"
#include "../components/ComponentRegistry.h"

/**
 * @file ComponentsAPI.cpp
 * @brief API pour exposer les définitions de composants au frontend
 * 
 * Routes:
 * - GET /api/components/definitions : Liste toutes les définitions de composants
 */

/**
 * @brief Initialise les routes API pour les composants
 * @param server Serveur web ESPAsyncWebServer
 */
void setupComponentsAPI(AsyncWebServer& server) {
    
    /**
     * GET /api/components/definitions
     * Retourne la liste des composants disponibles avec leurs métadonnées
     * 
     * Réponse:
     * [
     *   {"id":"potentiometer","displayName":"Potentiomètre","pinType":0,"implemented":true,"isComplex":false},
     *   {"id":"button","displayName":"Bouton","pinType":1,"implemented":true,"isComplex":false},
     *   ...
     * ]
     */
    server.on("/api/components/definitions", HTTP_GET, [](AsyncWebServerRequest* request) {
        // Allouer un buffer pour le JSON
        // Estimation: ~100 bytes par composant, max 10 composants = 1KB
        static char jsonBuffer[1024];
        
        int written = ComponentRegistry::toJsonArray(jsonBuffer, sizeof(jsonBuffer));
        
        if (written > 0) {
            request->send(200, "application/json", jsonBuffer);
        } else {
            request->send(500, "application/json", "{\"error\":\"Failed to generate component definitions\"}");
        }
    });
    
    /**
     * GET /api/components/count
     * Retourne le nombre de composants enregistrés
     */
    server.on("/api/components/count", HTTP_GET, [](AsyncWebServerRequest* request) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "{\"count\":%zu}", ComponentRegistry::count());
        request->send(200, "application/json", buffer);
    });
}
