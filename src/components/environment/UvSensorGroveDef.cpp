#include "UvSensorGroveDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_uv_sensor_grove = ComponentRegistry::registerDefinition(
    Components::UvSensorGrove::createDefinition()
);

