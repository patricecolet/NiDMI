#include "BarometerDps310Def.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_barometer_dps310 = ComponentRegistry::registerDefinition(
    Components::BarometerDps310::createDefinition()
);

