#include "UltrasonicDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_ultrasonic = ComponentRegistry::registerDefinition(
    Components::Ultrasonic::createDefinition()
);

