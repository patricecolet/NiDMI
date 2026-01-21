#include "RotaryAngleGroveDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_rotary_angle_grove = ComponentRegistry::registerDefinition(
    Components::RotaryAngleGrove::createDefinition()
);

