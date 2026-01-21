#include "EnvironmentGenericDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_environment_generic = ComponentRegistry::registerDefinition(
    Components::EnvironmentGeneric::createDefinition()
);

