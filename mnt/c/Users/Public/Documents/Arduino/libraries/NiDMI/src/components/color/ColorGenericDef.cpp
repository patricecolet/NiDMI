#include "ColorGenericDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_color_generic = ComponentRegistry::registerDefinition(
    Components::ColorGeneric::createDefinition()
);

