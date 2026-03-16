#include "GestureIrDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_gesture_ir = ComponentRegistry::registerDefinition(
    Components::GestureIr::createDefinition()
);

