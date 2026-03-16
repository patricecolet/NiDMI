#include "ThumbJoystickGroveDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_thumb_joystick_grove = ComponentRegistry::registerDefinition(
    Components::ThumbJoystickGrove::createDefinition()
);

