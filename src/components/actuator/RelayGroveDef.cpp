#include "RelayGroveDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_relay_grove = ComponentRegistry::registerDefinition(
    Components::RelayGrove::createDefinition()
);

