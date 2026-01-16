#include "APICommon.h"
#include "../components/ComponentRegistry.h"
#include "../managers/ComponentManager.h"
#include "../Globals.h"
#include <set>

/**
 * @file ComponentsAPI.cpp
 * @brief API pour exposer les définitions de composants au frontend
 * 
 * Routes:
 * - GET /api/components/definitions : Liste toutes les définitions de composants
 * - GET /api/components/used-gpios : Retourne les GPIOs utilisés par les composants configurés
 */

/**
 * @brief Initialise les routes API pour les composants
 * @param server Serveur web ESPAsyncWebServer
 */
void setupComponentsAPI(AsyncWebServer& server) {
    
    /**
     * GET /api/components/definitions
     * Retourne la liste des composants disponibles avec leurs métadonnées
     */
    server.on("/api/components/definitions", HTTP_GET, [](AsyncWebServerRequest* request) {
        // Buffer plus grand pour inclure les pins additionnelles
        static char jsonBuffer[4096];
        
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
    
    /**
     * GET /api/components/used-gpios
     * Retourne la liste des GPIOs utilisés par tous les composants configurés
     * 
     * Réponse:
     * {
     *   "gpios": [1, 2, 3, 4, 5, ...],
     *   "details": [
     *     {"gpio": 1, "componentId": "potentiometer", "pinLabel": "A0"},
     *     {"gpio": 2, "componentId": "mux", "pinId": "s0", "muxId": 0},
     *     ...
     *   ]
     * }
     */
    server.on("/api/components/used-gpios", HTTP_GET, [](AsyncWebServerRequest* request) {
        static char jsonBuffer[2048];
        int written = 0;
        
        // Créer un set des GPIOs utilisés
        std::set<uint8_t> usedGpios;
        
        // 1. Ajouter les GPIOs des composants simples
        uint8_t componentCount = g_componentManager.getComponentCount();
        for (uint8_t i = 0; i < componentCount; i++) {
            const ComponentConfig* cfg = g_componentManager.getConfig(i);
            if (cfg) {
                usedGpios.insert(cfg->gpio);
            }
        }
        
        // 2. Ajouter les GPIOs des MUX (SIG + S0-S3 + EN)
        uint8_t muxCount = g_componentManager.getMuxCount();
        for (uint8_t i = 0; i < muxCount; i++) {
            const MuxConfig* muxCfg = g_componentManager.getMuxConfig(i);
            if (muxCfg && muxCfg->enabled) {
                usedGpios.insert(muxCfg->sig_pin);
                usedGpios.insert(muxCfg->s0);
                usedGpios.insert(muxCfg->s1);
                usedGpios.insert(muxCfg->s2);
                usedGpios.insert(muxCfg->s3);
                if (muxCfg->en_pin != 255) {
                    usedGpios.insert(muxCfg->en_pin);
                }
            }
        }
        
        // Générer le JSON
        written = snprintf(jsonBuffer, sizeof(jsonBuffer), "{\"gpios\":[");
        
        bool first = true;
        for (uint8_t gpio : usedGpios) {
            if (!first) {
                written += snprintf(jsonBuffer + written, sizeof(jsonBuffer) - written, ",");
            }
            first = false;
            written += snprintf(jsonBuffer + written, sizeof(jsonBuffer) - written, "%d", gpio);
        }
        
        written += snprintf(jsonBuffer + written, sizeof(jsonBuffer) - written, "]}");
        
        request->send(200, "application/json", jsonBuffer);
    });
}
