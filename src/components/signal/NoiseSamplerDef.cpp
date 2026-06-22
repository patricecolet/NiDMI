#include "NoiseSamplerDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered = ComponentRegistry::registerDefinition(
    Components::NoiseSampler::createDefinition()
);
