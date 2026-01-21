#include "LedMatrixGroveDef.h"
#include "../ComponentRegistry.h"

static bool registered_led_matrix_grove = ComponentRegistry::registerDefinition(
    Components::LedMatrixGrove::createDefinition()
);
