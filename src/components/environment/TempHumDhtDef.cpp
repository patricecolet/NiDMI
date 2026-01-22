#include "TempHumDhtDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_temp_hum_dht = ComponentRegistry::registerDefinition(
    Components::TempHumDht::createDefinition()
);