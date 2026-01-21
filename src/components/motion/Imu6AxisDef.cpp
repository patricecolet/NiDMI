#include "Imu6AxisDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_imu_6axis = ComponentRegistry::registerDefinition(
    Components::Imu6Axis::createDefinition()
);

