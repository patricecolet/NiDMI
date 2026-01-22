#include "RadarDopplerDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_radar_doppler = ComponentRegistry::registerDefinition(
    Components::RadarDoppler::createDefinition()
);

