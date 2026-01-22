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
#ifdef NIDMI_COMPONENT_DEFS_PAGINATION
        // Mode pagination activé
        // Limit par défaut à 5 : avec buffer 12KB, 5 composants = ~9-10KB (marge confortable)
        // Cela permet de tester la pagination même avec peu de composants
        int page = 0, limit = 5;
        if (request->hasParam("page")) {
            page = request->getParam("page")->value().toInt();
            if (page < 0) page = 0;
        }
        if (request->hasParam("limit")) {
            limit = request->getParam("limit")->value().toInt();
            if (limit < 1) limit = 1;
            if (limit > 15) limit = 15; // Max 15 par page pour éviter les buffers trop grands (avec buffer 12KB)
        }
        
        static char jsonBuffer[12288];  // Buffer 12KB par page (augmenté pour éviter la troncature)
        int written = ComponentRegistry::toJsonArrayPage(jsonBuffer, sizeof(jsonBuffer), page, limit);
        
        if (written > 0 && written < (int)sizeof(jsonBuffer)) {
            // Ajouter les métadonnées de pagination dans les headers
            int totalCount = static_cast<int>(ComponentRegistry::count());
            int totalPages = (totalCount + limit - 1) / limit;
            
            #ifdef ARDUINO
            Serial.printf("[ComponentsAPI] Pagination: page=%d, limit=%d, totalCount=%d, totalPages=%d\n", 
                         page, limit, totalCount, totalPages);
            #endif
            
            AsyncWebServerResponse* response = request->beginResponse(200, "application/json", jsonBuffer);
            response->addHeader("X-Total-Count", String(totalCount));
            response->addHeader("X-Total-Pages", String(totalPages));
            response->addHeader("X-Current-Page", String(page));
            response->addHeader("X-Per-Page", String(limit));
            request->send(response);
        } else {
            Serial.printf("[ComponentsAPI] WARNING: Pagination serialization failed (written=%d, size=%zu)\n", 
                         written, sizeof(jsonBuffer));
            request->send(500, "application/json", "{\"error\":\"Serialization failed\"}");
        }
#else
        // Mode normal (sans pagination) - code existant
        // Buffer plus grand pour inclure toutes les métadonnées (MIDI params, formFields, etc.)
        // Avec les nouvelles structures, chaque composant peut prendre ~500-800 bytes
        // Pour ~10 composants, on a besoin d'au moins 8-10KB
        // Augmenté à 24KB pour permettre l'ajout de nouveaux composants (ultrasonic, etc.)
        static char jsonBuffer[24576];  // 24KB pour tester avec plusieurs composants
        
        int written = ComponentRegistry::toJsonArray(jsonBuffer, sizeof(jsonBuffer));
        
        #ifdef ARDUINO
        int totalCount = static_cast<int>(ComponentRegistry::count());
        Serial.printf("[ComponentsAPI] Mode normal: totalCount=%d, written=%d\n", totalCount, written);
        #endif
        
        if (written > 0 && written < (int)sizeof(jsonBuffer)) {
            request->send(200, "application/json", jsonBuffer);
        } else {
            // Si le buffer est trop petit, envoyer une erreur plutôt que de crasher
            Serial.printf("[ComponentsAPI] WARNING: Buffer too small or serialization failed (written=%d, size=%zu)\n", 
                         written, sizeof(jsonBuffer));
            request->send(500, "application/json", "{\"error\":\"Buffer too small for component definitions\"}");
        }
#endif
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
