#include "LidarTofDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_lidar_tof = ComponentRegistry::registerDefinition(
    Components::LidarTof::createDefinition()
);

