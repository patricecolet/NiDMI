#include "MuxDef.h"
#include "../../utils/PinMapper.h"

namespace Components {

bool MuxBase::validateSigPin(uint8_t gpio) {
    return PinMapper::hasAdc(gpio);
}

} // namespace Components
