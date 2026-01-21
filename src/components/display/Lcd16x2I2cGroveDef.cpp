#include "Lcd16x2I2cGroveDef.h"
#include "../ComponentRegistry.h"

static bool registered_lcd_16x2_i2c_grove = ComponentRegistry::registerDefinition(
    Components::Lcd16x2I2cGrove::createDefinition()
);
