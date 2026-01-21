#include "SolenoidGroveDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_solenoid_grove = ComponentRegistry::registerDefinition(
    Components::SolenoidGrove::createDefinition()
);

