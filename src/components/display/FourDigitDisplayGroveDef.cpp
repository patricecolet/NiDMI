#include "FourDigitDisplayGroveDef.h"
#include "../ComponentRegistry.h"

static bool registered_four_digit_display_grove = ComponentRegistry::registerDefinition(
    Components::FourDigitDisplayGrove::createDefinition()
);
