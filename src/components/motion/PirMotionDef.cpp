#include "PirMotionDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_pir_motion = ComponentRegistry::registerDefinition(
    Components::PirMotion::createDefinition()
);

