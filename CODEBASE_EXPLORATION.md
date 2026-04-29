# NiDMI Codebase Exploration Report

## 1. SEQUENCER API & PLAYBACK CONTROLS

### Core Files
- **[src/api/SequencerAPI.h](src/api/SequencerAPI.h)** - Main sequencer API setup
- **[src/api/SequencerAPI.cpp](src/api/SequencerAPI.cpp)** - Sequencer file management endpoints
- **[src/api/SequencerPlaybackAPI.h](src/api/SequencerPlaybackAPI.h)** - Playback control API
- **[src/api/SequencerPlaybackAPI.cpp](src/api/SequencerPlaybackAPI.cpp)** - Playback control endpoints
- **[src/processors/SequencerProcessor.h](src/processors/SequencerProcessor.h)** - Sequencer data structures and parsing

### Data Structures

```cpp
struct Note {
    uint8_t pitch;      // MIDI pitch (0-127)
    uint8_t velocity;   // MIDI velocity (0-127)
};

struct Step {
    uint8_t noteCount;  // Number of notes in this step (0-4)
    Note notes[MAX_NOTES];  // Up to 4 notes per step
    uint8_t measure;    // Which measure this step belongs to
};

struct SequencerState {
    char currentFile[64];       // Currently loaded .nidmid file
    uint8_t currentMeasure;     // Current measure (1-indexed)
    uint8_t stepIndex;          // Current step in sequence (0-indexed)
    uint8_t totalMeasures;      // Total measures in current sequence
};

extern Step steps[32];          // Max 32 steps
extern uint8_t stepCount;       // Number of loaded steps
```

### Sequencer Playback Control Methods

#### 1. **File Management**
```
GET  /api/files
  → Returns: ["file1.nidmid", "file2.nidmid", ...]
  → Lists all available .nidmid files in /seq directory

POST /api/select
  Body: {"file":"filename.nidmid"}
  → Loads and parses the specified sequencer file
  → Returns: {"ok":true, "file":"...", "totalMeasures":N}
  → Key function: parseNidmid(uint8_t* data, size_t len)
  → Parses binary .nidmid format into steps[] array
```

#### 2. **Playback Status & Navigation**
```
GET  /api/status
  → Returns: {"file":"...", "measure":N, "totalMeasures":N, 
              "stepIndex":N, "totalSteps":N}

POST /api/measure
  Body: {"measure":N}
  → Jumps to specified measure (1-indexed)
  → Helper: int findFirstStepInMeasure(uint8_t measure)
  → Updates: currentMeasure, stepIndex

POST /api/reset
  → Resets playback to start (measure=1, stepIndex=0)
```

#### 3. **Step Playback**
```
POST /api/step
  → Plays the current step and advances to next
  → Returns: {
      "ok": true,
      "note": MIDI_pitch,
      "velocity": MIDI_velocity,
      "channel": 1,
      "measure": current_measure,
      "stepIndex": current_step,
      "totalSteps": total,
      "done": true/false,           // true when sequence ends
      "notes": [                     // if step has multiple notes
        {"pitch": N, "velocity": N},
        ...
      ]
    }
  → Helper: int getNextStepIndex() - gets next step or -1 if at end
  → Helper: uint8_t calculateTotalMeasures() - calculates max measure
```

### Upload Endpoints
```
POST /api/sequencer/upload
  → Handles chunked binary file upload
  → Callbacks: beginUpload(), appendChunk(), endUpload()
  → Auto-reloads via reloadSequencerFromStorage()
  
GET  /api/sequencer/download
  → Downloads current sequencer data as .nidmid file

GET  /api/sequencer/view
  → Returns JSON view of all loaded steps
```

### Key Processing Functions

**parseNidmid(uint8_t* data, size_t len)**
- Parses binary .nidmid file format
- Binary format markers:
  - `0xFF`: New measure marker
  - `0x00`: No notes in this step
  - `0x01-0x04`: Number of notes following
  - Each note: [pitch_byte, velocity_byte]

**reloadSequencerFromStorage()**
- Reloads sequencer from LittleFS file (/seq/nidmid.bin)
- Validates CRC32 if available in header
- Returns: true if successful, false if file absent/invalid
- Safe to call at any time during runtime (hot-reload)

**Global State Variables**
```cpp
extern Step steps[32];          // Loaded step data
extern uint8_t stepCount;       // Count of loaded steps
static SequencerState g_playbackState;  // Current playback position
```

---

## 2. HARDWARE BUTTONS/INPUTS HANDLING

### Button Definition & Configuration
**[src/components/basic/ButtonDef.h](src/components/basic/ButtonDef.h)**

```cpp
struct ButtonConfig {
    char btnMode[16];         // "pulse", "press_release", or "toggle"
    char btnPulseTiming[16];  // "press" or "release" (for pulse mode)
    char btnPullMode[16];     // "pullup", "pulldown", or "none"
};

// Button::createDefinition() provides form fields for:
// - Mode selection (3-way button modes)
// - Pulse timing (when to trigger in pulse mode)
// - Pull resistor configuration
// - MIDI message type support: Note, CC, Program Change, Clock
```

### Button Processing
**[src/processors/ButtonProcessor.h](src/processors/ButtonProcessor.h)**
**[src/processors/ButtonProcessor.cpp](src/processors/ButtonProcessor.cpp)**

**Main Function: `ButtonProcessor::process()`**

```cpp
static void process(
    const ComponentConfig& config,
    ComponentState& state,
    MidiSender* midi_sender,
    OSCQueue& osc_queue
);
```

**Processing Flow:**
1. **Digital Read** - `digitalRead(config.gpio)` with pull-mode detection
   - Pull-up mode: LOW = pressed
   - Pull-down mode: HIGH = pressed

2. **Debouncing** - 50ms debounce timer
   - `state.last_change_time` tracks transition timing
   - `state.prev_stable_state` stores stable state after debounce
   - Returns early if not yet stable

3. **Edge Detection**
   - `falling`: Transition from released (false) → pressed (true) = PRESS
   - `rising`: Transition from pressed (true) → released (false) = RELEASE
   - Only processes on these edges

4. **Mode Handling** (configured via btnMode)
   
   **PRESS_RELEASE Mode:**
   - On press (falling edge): sends Note On / CC On
   - On release (rising edge): sends Note Off / CC Off
   
   **PULSE Mode:**
   - Triggers on either press or release (configurable via btnPulseTiming)
   - Sends single MIDI message
   
   **TOGGLE Mode:**
   - Alternates between on/off on successive presses

5. **MIDI Output** (via MidiOutputCoordinator)
   ```cpp
   MidiOutputCoordinator::sendMidiAndOsc(midi_sender, osc_queue, config, value);
   ```
   - Sends MIDI Note On/Off, CC, Program Change, or Clock based on `config.msg_type`
   - Also sends OSC messages if configured

6. **Script Mode Support**
   - In script mode, button armed after 300ms stabilization
   - Suppresses startup/floating transients
   - Edge-driven behavior independent of btnMode

### Component Input Flow Architecture

**[src/managers/ComponentManager.h](src/managers/ComponentManager.h)**
**[src/managers/ComponentManager.cpp](src/managers/ComponentManager.cpp)**

```
FreeRTOS Task: MidiTask (Core 0)
├── Runs midiTaskLoop() at high priority
├── Reads all digital inputs (buttons, touch, etc.)
├── Calls ButtonProcessor::process() for each button
├── Sends MIDI messages via g_midiRouter
└── Updates FreeRTOS telemetry queue

Main Loop (Core 1): nidmi.loop()
├── Handles HTTP requests
├── Processes OSC messages
├── Sends WebSocket telemetry
└── Communicates with Core 0 via queues
```

### Component Registration & Processing

**[src/components/ComponentRegistry.h](src/components/ComponentRegistry.h)**
- `ComponentRegistry::init()` - Initializes all available components
- `ComponentRegistry::getAll()` - Gets all registered component definitions
- `ComponentRegistry::findById(id)` - Finds component by ID string

**Supported Input Components:**
- **Button** (digital) - press/release/pulse/toggle modes
- **Potentiometer** (analog) - continuous MIDI CC
- **Joystick** (analog X/Y) - dual-axis input
- **Touch** (capacitive, ESP32-S3) - touch-sensitive pins
- **Velostat** (analog pressure)
- **Ultrasonic** (distance sensor)
- **FSR** (force-sensitive resistor)
- **Mpr121** (12-channel capacitive touch IC)
- **RotaryAngleGrove** (rotary sensor)
- **ThumbJoystickGrove** (thumb joystick)

---

## 3. MAIN SKETCH SETUP & COMPONENT INITIALIZATION

### Main Sketch File
**[examples/nidmi_basic/nidmi_basic.ino](examples/nidmi_basic/nidmi_basic.ino)**

```cpp
#include <NiDMI.h>

void setup() {
    Serial.begin(115200);
    delay(100);
    
    // Single call initializes everything:
    nidmi.begin();
    
    // System now ready:
    // - WiFi AP "nidmi" active (password: "nidmipass")
    // - Web interface on http://192.168.4.1
    // - RTP-MIDI initialized
    // - ComponentManager ready for configurations
}

void loop() {
    // All processing handled automatically:
    nidmi.loop();
    
    // ComponentManager manages:
    // - Reading potentiometers/buttons
    // - Debouncing
    // - Automatic MIDI sending
    // - Configuration reloading
}
```

### Initialization Chain (`nidmi.begin()`)

**[src/NiDMI.cpp](src/NiDMI.cpp) - `nidmi_begin()` function**

1. **Serial Setup**
   - `Serial.begin(115200)`

2. **LittleFS Initialization**
   - Mounts main LittleFS partition
   - Optionally mounts `/seqfs` (sequencer storage) if partition exists
   - Optionally mounts `/mapfs` (mapping storage) if partition exists

3. **MCU Detection**
   - `PinMapper::detectMcu()` - Detects ESP32-C3 vs ESP32-S3
   - `PinMapper::printMappings()` - Logs pin availability

4. **NVS Configuration Loading**
   - Reads from Preferences namespace "nidmi":
     - `mdns_name` - mDNS hostname
     - `sta_ssid`, `sta_pass` - WiFi STA credentials
     - `sta_ip`, `sta_gw`, `sta_sn` - Static IP config
     - `touch_enabled` - Enable/disable touch processing
     - `usbmidi_enabled` - Enable/disable USB-MIDI

5. **WiFi & Server Initialization**
   - `serverCore.begin()` - Starts WiFi AP and web server
     - AP SSID: configured name (default "nidmi")
     - AP Password: "nidmipass"
     - Web interface: http://192.168.4.1
   - `serverCore.connectSta()` - Connects to configured WiFi if STA credentials saved

6. **ComponentManager Initialization**
   - `g_componentManager.begin(midi_sender)`
   - Loads saved component configurations from NVS
   - Starts FreeRTOS MIDI task on Core 0
   - Initializes MuxManager for analog multiplexers

7. **API Setup**
   - `setupSequencerAPI()` - Sequencer file upload/download endpoints
   - `setupSequencerPlaybackAPI()` - Playback control endpoints
   - Other APIs (Network, System, OSC, etc.)

### Main Loop Processing (`nidmi.loop()`)

**[src/NiDMI.cpp](src/NiDMI.cpp) - `nidmi_loop()` function**

```
Main Loop (runs on Core 1):
├── Check for deferred reload pins request (500ms debounce)
├── Check for deferred reboot request
├── Process STA reconnection (10s interval if disconnected)
├── ComponentManager::update()
│   ├── Sync OSC configuration
│   ├── Process OSC incoming messages
│   ├── Send OSC batches from MUX
│   ├── Process OSC queue
│   └── Drain telemetry WebSocket messages (from MIDI task on Core 0)
└── Server processing (implicit)
```

### Key Component Initialization Functions

**ComponentManager::begin()**
```cpp
void ComponentManager::begin(MidiSender* sender) {
    // 1. Load multiplexer configs from NVS
    loadMuxConfigFromNVS();
    
    // 2. Load component pin configs from NVS
    ConfigLoader::loadFromNVS(*this);
    
    // 3. Initialize OSC from NVS
    OSCConfigLoader::OSCConfig oscConfig = OSCConfigLoader::loadFromNVS();
    OSCConfigLoader::initialize(oscConfig, osc_manager, osc_queue, ...);
    
    // 4. Start MUX task on Core 0
    mux_manager.begin();
    
    // 5. Start MIDI/Component processing task on Core 0
    xTaskCreatePinnedToCore(midiTask, "MidiTask", 8192, this, 4, &midiTaskHandle, 0);
    
    // 6. Print statistics
    printStats();
}
```

**ComponentInitializer Functions** - Initialize default config/state

```cpp
ComponentInitializer::initializeConfig()   // Set default MIDI params
ComponentInitializer::initializeState()    // Zero out state variables
ComponentInitializer::setupGpio()          // Configure GPIO mode (INPUT, INPUT_PULLUP, etc.)
```

---

## 4. INTEGRATION PATTERNS & KEY FLOW

### Configuration Persistence
```
NVS (Preferences) Storage
├── Namespace "nidmi" - System config (WiFi, mDNS, touch enabled)
└── Component configs - Each GPIO/component stores:
    - Type (button, potentiometer, etc.)
    - MIDI channel & parameter
    - Message type (note, CC, PC)
    - Component-specific settings (btnMode, pullMode, etc.)
```

### MIDI Message Routing
```
Button Press
├── ButtonProcessor::process() detects edge
├── Determines MIDI message type (Note/CC/PC/Clock)
├── Calls MidiOutputCoordinator::sendMidiAndOsc()
├── Routes to MidiRouter::sendNoteOn() or sendControlChange()
└── Outputs via:
    - USB MIDI (if TinyUSB configured)
    - RTP-MIDI (network MIDI)
    - OSC (if configured)
```

### Real-time Task Architecture
```
Core 0 (Real-time):
├── MuxTask - Reads analog multiplexers, sends OSC batches
└── MidiTask - Processes all digital inputs, sends MIDI

Core 1 (Network):
├── Main loop - HTTP server, OSC processing, telemetry
└── WiFi driver
```

---

## 5. KEY FUNCTIONS QUICK REFERENCE

### Sequencer Playback
| Function | Location | Purpose |
|----------|----------|---------|
| `parseNidmid()` | SequencerProcessor.h | Parse binary .nidmid format |
| `reloadSequencerFromStorage()` | SequencerProcessor.h | Load from LittleFS |
| `getNextStepIndex()` | SequencerPlaybackAPI.cpp | Get next step index |
| `findFirstStepInMeasure()` | SequencerPlaybackAPI.cpp | Find step in measure |
| `calculateTotalMeasures()` | SequencerPlaybackAPI.cpp | Count measures |

### Button Input
| Function | Location | Purpose |
|----------|----------|---------|
| `ButtonProcessor::process()` | ButtonProcessor.cpp | Main button processing |
| `digitalRead()` | Arduino | Read button state |
| Debounce timer | ButtonProcessor.cpp | 50ms debounce |
| Edge detection | ButtonProcessor.cpp | Falling/rising transitions |

### Component Management
| Function | Location | Purpose |
|----------|----------|---------|
| `ComponentManager::begin()` | ComponentManager.cpp | Initialize all components |
| `ComponentManager::addComponent()` | ComponentManager.cpp | Register new component |
| `ComponentRegistry::init()` | ComponentRegistry.cpp | Load all component definitions |
| `ConfigLoader::loadFromNVS()` | ConfigLoader.h | Load saved configurations |

### System Initialization
| Function | Location | Purpose |
|----------|----------|---------|
| `nidmi_begin()` | NiDMI.cpp | System initialization (called by nidmi.begin()) |
| `nidmi_loop()` | NiDMI.cpp | Main processing loop (called by nidmi.loop()) |
| `ServerCore::begin()` | ServerCore.h | Start WiFi AP + web server |
| `PinMapper::detectMcu()` | PinMapper.h | Detect ESP32-C3 or ESP32-S3 |

---

## Summary

**Sequencer:** File-based playback with measure/step navigation via HTTP endpoints. Binary .nidmid format with multi-note support per step.

**Button Input:** Debounced digital input with configurable modes (press/release, pulse, toggle). Sends MIDI via USB/RTP-MIDI/OSC.

**Initialization:** Single `nidmi.begin()` call sets up WiFi, LittleFS, ComponentManager, and APIs. Full configuration via web UI at http://192.168.4.1. Automatic MIDI processing in background FreeRTOS task.
