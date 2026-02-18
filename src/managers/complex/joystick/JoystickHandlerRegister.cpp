/**
 * @file JoystickHandlerRegister.cpp
 * @brief Enregistrement automatique du JoystickHandler
 * 
 * Fichier séparé pour éviter les dépendances circulaires lors de l'inclusion de ComplexHandlerRegistry.h
 */

#include "JoystickHandler.h"

/* ComplexHandlerRegistry est nécessaire directement ici pour l'enregistrement */
#include "../ComplexHandlerRegistry.h"

static ComplexHandler* createJoystickHandler() {
    return new JoystickHandler();
}

// Enregistrement automatique au chargement
static bool registered_joystick = ComplexHandlerRegistry::registerHandler("joystick", createJoystickHandler);
