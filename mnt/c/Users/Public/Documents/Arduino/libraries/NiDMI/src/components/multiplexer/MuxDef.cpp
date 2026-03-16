#include "MuxDef.h"
#include "../ComponentRegistry.h"
#include "../../utils/PinMapper.h"

namespace Components {

bool MuxBase::validateSigPin(uint8_t gpio) {
    return PinMapper::hasAdc(gpio);
}

} // namespace Components

// Enregistrement automatique au chargement du module
// Enregistrer HC4067
static bool registered_hc4067 = ComponentRegistry::registerDefinition(
    Components::HC4067::createDefinition()
);

// Enregistrer HC4051
static bool registered_hc4051 = ComponentRegistry::registerDefinition(
    Components::HC4051::createDefinition()
);

