#include "APICommon.h"
#include "../components/ComponentRegistry.h"
#include "../components/ComponentTypes.h"
#include "../components/motion/Lis3dhDef.h"
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
        int page = 0, limit = 5;
        if (request->hasParam("page")) {
            page = request->getParam("page")->value().toInt();
            if (page < 0) page = 0;
        }
        if (request->hasParam("limit")) {
            limit = request->getParam("limit")->value().toInt();
            if (limit < 1) limit = 1;
            if (limit > 15) limit = 15;
        }
        
        /* Buffer heap (pas static) : évite toute corruption si deux requêtes se chevauchent
         * et permet de détecter un OOM au lieu d'envoyer un body vide silencieusement. */
        const size_t bufSize = 16384;
        char* jsonBuffer = (char*)malloc(bufSize);
        if (!jsonBuffer) {
            Serial.println("[ComponentsAPI] ERROR: malloc pagination buffer failed");
            request->send(503, "application/json", "{\"error\":\"Out of memory\"}");
            return;
        }
        
        int written = ComponentRegistry::toJsonArrayPage(jsonBuffer, bufSize, page, limit);
        
        if (written > 0 && written < (int)bufSize) {
            int totalCount = static_cast<int>(ComponentRegistry::count());
            int totalPages = (totalCount + limit - 1) / limit;
            
            /* Copier dans String et libérer le buffer heap immédiatement
             * (beginResponse copierait aussi, mais ici on détecte un OOM String) */
            String body;
            if (!body.reserve(written + 1)) {
                free(jsonBuffer);
                Serial.printf("[ComponentsAPI] ERROR: String reserve OOM (%d bytes)\n", written);
                request->send(503, "application/json", "{\"error\":\"Out of memory\"}");
                return;
            }
            body = jsonBuffer;
            free(jsonBuffer);
            jsonBuffer = nullptr;
            
            #ifdef ARDUINO
            Serial.printf("[ComponentsAPI] page=%d/%d, count=%d, %u bytes\n",
                         page, totalPages - 1, totalCount, body.length());
            #endif
            
            AsyncWebServerResponse* response = request->beginResponse(200, "application/json", body);
            response->addHeader("X-Total-Count", String(totalCount));
            response->addHeader("X-Total-Pages", String(totalPages));
            response->addHeader("X-Current-Page", String(page));
            response->addHeader("X-Per-Page", String(limit));
            request->send(response);
        } else {
            free(jsonBuffer);
            Serial.printf("[ComponentsAPI] WARNING: Pagination serialization failed (written=%d, bufSize=%zu)\n",
                         written, bufSize);
            request->send(500, "application/json", "{\"error\":\"Serialization failed\"}");
        }
#else
        /* Mode normal (sans pagination)
         * Buffer 32 Ko pour les définitions avec MIDI params, formFields, etc.
         * Chaque composant peut prendre ~500-2000 bytes selon sa complexité. */
        static char jsonBuffer[32768];

        int written = ComponentRegistry::toJsonArray(jsonBuffer, sizeof(jsonBuffer));

        /* Sécurité : s'assurer que written correspond au contenu réel (strlen).
         * Protège contre un compteur written désynchronisé du buffer réel
         * (par ex. si snprintf a tronqué mais que toJson n'a pas détecté). */
        size_t actualLen = strlen(jsonBuffer);
        if (written > 0 && actualLen > 0 && actualLen < sizeof(jsonBuffer)) {
            if ((size_t)written != actualLen) {
                Serial.printf("[ComponentsAPI] WARNING: written=%d != strlen=%zu, utilisation strlen\n", written, actualLen);
            }
            /* Envoyer avec la longueur réelle (strlen) via String pour éviter d'envoyer du garbage */
            request->send(200, "application/json", String(jsonBuffer));
        } else {
            Serial.printf("[ComponentsAPI] WARNING: Serialization failed (written=%d, strlen=%zu, bufSize=%zu)\n",
                         written, actualLen, sizeof(jsonBuffer));
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
        
        // 1. Ajouter les GPIOs des composants simples + GPIOs spécifiques (CS pour SPI IMU)
        uint8_t componentCount = g_componentManager.getComponentCount();
        for (uint8_t i = 0; i < componentCount; i++) {
            const ComponentConfig* cfg = g_componentManager.getConfig(i);
            if (cfg) {
                usedGpios.insert(cfg->gpio);
                if (cfg->type == ComponentType::IMU && cfg->specificConfig.imu) {
                    uint8_t cs = cfg->specificConfig.imu->cs_gpio;
                    if (cs != 255) usedGpios.insert(cs);
                }
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
