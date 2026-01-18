#include "ComplexHandlerRegistry.h"

/* Note: Les fichiers d'enregistrement (*Register.cpp) sont compilés séparément par le système de build */
/* L'enregistrement se fera automatiquement au chargement du module via les variables statiques */

ComplexHandlerRegistry::HandlerEntry ComplexHandlerRegistry::handlers[ComplexHandlerRegistry::MAX_HANDLERS];
uint8_t ComplexHandlerRegistry::handler_count = 0;
bool ComplexHandlerRegistry::initialized = false;

bool ComplexHandlerRegistry::registerHandler(const char* componentId, ComplexHandlerFactory factory) {
    if (!componentId || !factory || handler_count >= MAX_HANDLERS) {
        return false;
    }
    
    /* Vérifier si déjà enregistré */
    for (uint8_t i = 0; i < handler_count; i++) {
        if (handlers[i].componentId && strcmp(handlers[i].componentId, componentId) == 0) {
            return false;  /* Déjà enregistré */
        }
    }
    
    /* Ajouter au registre */
    handlers[handler_count].componentId = componentId;
    handlers[handler_count].factory = factory;
    handlers[handler_count].instance = nullptr;  /* Créé à la demande */
    handler_count++;
    
    return true;
}

ComplexHandler* ComplexHandlerRegistry::getHandler(const char* componentId) {
    if (!componentId) {
        return nullptr;
    }
    
    /* Initialiser les instances si nécessaire */
    if (!initialized) {
        init();
    }
    
    /* Chercher le handler */
    for (uint8_t i = 0; i < handler_count; i++) {
        if (handlers[i].componentId && strcmp(handlers[i].componentId, componentId) == 0) {
            /* Créer l'instance si nécessaire (singleton par type) */
            if (!handlers[i].instance) {
                handlers[i].instance = handlers[i].factory();
            }
            return handlers[i].instance;
        }
    }
    
    return nullptr;
}

bool ComplexHandlerRegistry::hasHandler(const char* componentId) {
    return getHandler(componentId) != nullptr;
}

void ComplexHandlerRegistry::init() {
    /* Les instances sont créées à la demande dans getHandler() */
    initialized = true;
}

ComplexHandler* ComplexHandlerRegistry::getHandlerByIndex(uint8_t index) {
    if (index >= handler_count) {
        return nullptr;
    }
    
    /* Initialiser les instances si nécessaire */
    if (!initialized) {
        init();
    }
    
    /* Créer l'instance si nécessaire */
    if (!handlers[index].instance) {
        handlers[index].instance = handlers[index].factory();
    }
    
    return handlers[index].instance;
}
