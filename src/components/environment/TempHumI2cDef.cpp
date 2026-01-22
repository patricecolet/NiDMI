#include "TempHumI2cDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_temp_hum_i2c = ComponentRegistry::registerDefinition(
    Components::TempHumI2c::createDefinition()
);

