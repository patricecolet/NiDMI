#include "IrDistanceSharpDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_ir_distance_sharp = ComponentRegistry::registerDefinition(
    Components::IrDistanceSharp::createDefinition()
);

