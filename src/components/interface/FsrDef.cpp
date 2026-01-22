#include "FsrDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_fsr = ComponentRegistry::registerDefinition(
    Components::Fsr::createDefinition()
);
