#include "VibrationMotorGroveDef.h"
#include "../ComponentRegistry.h"

// Enregistrement automatique au chargement du module
static bool registered_vibration_motor_grove = ComponentRegistry::registerDefinition(
    Components::VibrationMotorGrove::createDefinition()
);

