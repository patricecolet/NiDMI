#include "MotionGenericDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_motion_generic = ComponentRegistry::registerDefinition(
    Components::MotionGeneric::createDefinition()
);

