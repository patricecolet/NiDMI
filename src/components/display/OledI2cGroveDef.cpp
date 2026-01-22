#include "OledI2cGroveDef.h"
#include "../ComponentRegistry.h"

static bool registered_oled_i2c_grove = ComponentRegistry::registerDefinition(
    Components::OledI2cGrove::createDefinition()
);
