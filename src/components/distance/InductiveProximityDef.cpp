#include "InductiveProximityDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_inductive_proximity = ComponentRegistry::registerDefinition(
    Components::InductiveProximity::createDefinition()
);

