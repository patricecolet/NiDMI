#include "LightSensorGroveDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_light_sensor_grove = ComponentRegistry::registerDefinition(
    Components::LightSensorGrove::createDefinition()
);

