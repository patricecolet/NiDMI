#include "BuzzerGroveDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_buzzer_grove = ComponentRegistry::registerDefinition(
    Components::BuzzerGrove::createDefinition()
);

