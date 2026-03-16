#include "ProcessorRegistry.h"
#include "../components/ComponentTypes.h"  // Définitions communes
#include "../utils/AnalogFilter.h"

// Table de registre : MAX_COMPONENT_TYPES = 256 (uint8_t)
static ProcessorRegistry::ProcessorFunc processor_table[256] = {nullptr};

bool ProcessorRegistry::registerProcessor(ComponentType type, ProcessorFunc processor) {
    if (processor == nullptr) {
        return false;
    }
    
    uint8_t type_index = static_cast<uint8_t>(type);
    if (type_index >= 256) {
        return false;
    }
    
    processor_table[type_index] = processor;
    return true;
}

bool ProcessorRegistry::process(
    ComponentType type,
    const ComponentConfig& config,
    ComponentState& state,
    AnalogFilter* filter,
    MidiSender* midi_sender,
    OSCQueue& osc_queue
) {
    uint8_t type_index = static_cast<uint8_t>(type);
    if (type_index >= 256) {
        return false;
    }
    
    ProcessorFunc processor = processor_table[type_index];
    if (processor == nullptr) {
        return false;
    }
    
    processor(config, state, filter, midi_sender, osc_queue);
    return true;
}

bool ProcessorRegistry::hasProcessor(ComponentType type) {
    uint8_t type_index = static_cast<uint8_t>(type);
    if (type_index >= 256) {
        return false;
    }
    return processor_table[type_index] != nullptr;
}
