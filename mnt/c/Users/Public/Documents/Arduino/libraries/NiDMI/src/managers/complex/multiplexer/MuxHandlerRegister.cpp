/**
 * @file MuxHandlerRegister.cpp
 * @brief Enregistrement automatique du MuxHandler
 * 
 * Fichier séparé pour éviter les dépendances circulaires lors de l'inclusion de ComplexHandlerRegistry.h
 */

#include "MuxHandler.h"
/* ComplexHandlerRegistry est nécessaire directement ici pour l'enregistrement */
/* Utiliser un chemin relatif à src/ comme dans PinAPI.cpp */
#include "../ComplexHandlerRegistry.h"

/* Factory function pour créer une instance de MuxHandler */
static ComplexHandler* createMuxHandler() {
    return new MuxHandler(nullptr);  /* MuxManager sera accessible via g_componentManager */
}

/* Enregistrement automatique au chargement du module */
static bool registered_hc4067 = ComplexHandlerRegistry::registerHandler("hc4067", createMuxHandler);
static bool registered_hc4051 = ComplexHandlerRegistry::registerHandler("hc4051", createMuxHandler);
