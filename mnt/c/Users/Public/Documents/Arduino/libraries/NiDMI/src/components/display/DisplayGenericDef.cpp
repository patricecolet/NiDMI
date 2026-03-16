#include "DisplayGenericDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_display_generic = ComponentRegistry::registerDefinition(
    Components::DisplayGeneric::createDefinition()
);

