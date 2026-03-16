#include "ActuatorGenericDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_actuator_generic = ComponentRegistry::registerDefinition(
    Components::ActuatorGeneric::createDefinition()
);

