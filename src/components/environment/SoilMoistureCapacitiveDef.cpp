#include "SoilMoistureCapacitiveDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_soil_moisture_capacitive = ComponentRegistry::registerDefinition(
    Components::SoilMoistureCapacitive::createDefinition()
);

