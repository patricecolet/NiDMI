#include "Mpr121Def.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_mpr121 = ComponentRegistry::registerDefinition(
    Components::Mpr121::createDefinition()
);
