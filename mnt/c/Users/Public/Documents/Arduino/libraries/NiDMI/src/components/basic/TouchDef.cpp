#include "TouchDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_touch = ComponentRegistry::registerDefinition(
    Components::Touch::createDefinition()
);
