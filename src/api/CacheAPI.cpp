#include "APICommon.h"

void setupCacheAPI(AsyncWebServer& server) {
    /* API - Forcer la sauvegarde du cache en NVS */
    server.on("/api/cache/save", HTTP_POST, [](AsyncWebServerRequest *request){
        debug_network( "[CacheAPI] forceSave() appelé\n");
        g_configCache.forceSave();
        debug_network( "[CacheAPI] forceSave() terminé\n");
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
    
    /* API - Clear complet de la NVS (utile pour réinitialiser) */
    server.on("/api/cache/clear", HTTP_POST, [](AsyncWebServerRequest *request){
        Serial.println("[CacheAPI] Clear NVS demandé via API");
        g_configCache.clearAllNVS();
        request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"NVS cleared\"}");
    });
}
