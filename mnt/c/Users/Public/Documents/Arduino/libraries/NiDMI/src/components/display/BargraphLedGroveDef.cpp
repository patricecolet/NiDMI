#include "BargraphLedGroveDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_bargraph_led_grove = ComponentRegistry::registerDefinition(
    Components::BargraphLedGrove::createDefinition()
);
