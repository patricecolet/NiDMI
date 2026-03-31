#include "Lis3dhDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_lis3dh = ComponentRegistry::registerDefinition(
    Components::Lis3dh::createDefinition()
);
