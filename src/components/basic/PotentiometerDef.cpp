#include "PotentiometerDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered = ComponentRegistry::registerDefinition(
    Components::Potentiometer::createDefinition()
);
