/**
 * @file Joystick3HandlerRegister.cpp
 * @brief Enregistrement automatique du Joystick3Handler
 *
 * Fichier séparé pour éviter les dépendances circulaires lors de l'inclusion de ComplexHandlerRegistry.h
 */

#include "Joystick3Handler.h"

/* ComplexHandlerRegistry est nécessaire directement ici pour l'enregistrement */
#include "../ComplexHandlerRegistry.h"

static ComplexHandler* createJoystick3Handler() {
    return new Joystick3Handler();
}

// Enregistrement automatique au chargement
static bool registered_joystick3 = ComplexHandlerRegistry::registerHandler("joystick3", createJoystick3Handler);
