#include "Joystick3Def.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered = ComponentRegistry::registerDefinition(
    Components::Joystick3::createDefinition()
);
