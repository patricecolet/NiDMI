# Physical Button Mapping for Sequencer Play Step

This guide explains how to use a physical button connected to the ESP32 to control the sequencer's play step function.

## Feature Overview

When you:
1. Connect a physical button to a GPIO pin on the ESP32
2. Configure it in the pins interface with the "Sequencer Play Step" mapping
3. Each time you press the button, it triggers the sequencer to play the next step in the loaded NIDMI file

## Setup Instructions

### Step 1: Connect Physical Button to ESP32

Connect a push button to one of the available GPIO pins on your ESP32. For example:
- Button one pin: **GPIO pin (e.g., D0, D1, D2, D3)**
- Button other pin: **GND**

### Step 2: Configure the Button in NiDMI Web Interface

1. Open the **NiDMI web interface** (http://esp32.local or http://192.168.x.x)
2. Click on the GPIO **pin label** (e.g., **D0**) where you connected the button
3. The **Pin Editor** will open

### Step 3: Select Button Role

1. Find the **"Role"** dropdown in the form
2. Select **"Button"** from the list
3. This configures the pin as a digital input button

### Step 4: Configure Button Mode

Once "Button" is selected, you'll see button-specific options:
- **Button Mode**: Select **"Push"** (triggers on button press)
- **Timing**: Select **"Au press"** (on press) or **"Au release"** (on release)
- **Pull Mode**: Select **"Pull-up"** (standard for active-low buttons)

### Step 5: Select MIDI Mode

Choose how the button sends data:
- **RTP-MIDI**: Standard MIDI control (for MIDI devices)
- **Mapping Script**: Custom mapping (required for sequencer control)

Select **"Mapping Script"** radio button.

### Step 6: Apply Sequencer Play Step Mapping

1. You'll see a **"Mapping"** section with a dropdown labeled **"Quick Template"**
2. Click the dropdown and select **"Sequencer Play Step"**
3. This automatically fills in the mapping script: `seq.out("play_step")`

```
Quick Template: [Sequencer Play Step ▼]
Mapping Script: seq.out("play_step")
```

### Step 7: Save Configuration

Click **"Save"** button to save the button configuration to the ESP32.

## Testing

### Test 1: Manual Button Control

1. Navigate to the **Sequencer** page (http://esp32.local/sequencer.html)
2. Load a `.nidmid` file using the "Select file" dropdown
3. Press your **physical button**
4. ✅ You should see:
   - The button flash in the UI
   - A new step plays (note appears in the log)
   - The progress bar advances

### Test 2: Verify WebSocket Connection

1. Open browser **Developer Tools** (F12)
2. Go to **Console** tab
3. Look for messages like:
   ```
   [SequencerWebSocket] Connected
   Button pressed → play step
   ```

### Test 3: Button Press Behavior

- **Each button press** = Play **one step** in the sequence
- **Sequence loops** when you reach the end
- **Reset button** in UI resets to the start

## Mapping Script Syntax

The `seq.out()` function sends events to the sequencer page via WebSocket.

### Basic Usage
```
seq.out("play_step")  // Plays next step when button pressed
```

### How It Works

When you press the button:
1. The MappingEngine executes: `seq.out("play_step")`
2. Sends JSON via WebSocket: `{"type":"seq_event","source":"play_step","value":1}`
3. Sequencer.html receives the message
4. Automatically calls `stepPlay()` function
5. Next step in sequence plays

## Advanced: Custom Mapping Scripts

If you want more control, you can write custom mapping scripts:

### Example: MIDI Note + Sequencer Step

Play a MIDI note AND advance the sequencer:
```
note.out(60,1):seq.out("play_step")
```

### Example: With Different Button Modes

**Press-Release Mode** (plays on button release):
```
seq.out("play_step")
```

**Velocity from Analog Input** (if using potentiometer):
```
r("volume"):*(127):seq.out("play_step")
```

## Troubleshooting

### Issue: Button doesn't trigger sequencer step

**Check 1**: Verify button is physically connected and working
- Press button and check for console messages in browser dev tools

**Check 2**: Verify mapping script is set correctly
- Go back to pin editor
- Check that "Mapping Script" mode is selected (not RTP-MIDI)
- Verify script shows: `seq.out("play_step")`

**Check 3**: Check WebSocket connection
- Open browser Dev Tools → Network → WS
- You should see `/ws` connection open
- If not, WebSocket is not connecting - check ESP32 network status

**Check 4**: Verify sequencer page is open
- The WebSocket listener runs on `sequencer.html`
- Make sure you're viewing the sequencer page
- Refresh the page if needed

### Issue: Button works but no visual feedback

**Check 1**: Click "Load" to load a .nidmid file first
- Sequencer needs a file loaded to play steps

**Check 2**: Check note log
- Each step play should show in the "Note log" section
- Look for "Button pressed → play step" messages

### Issue: Multiple buttons interfere with each other

**Solution**: Each button can use `seq.out("play_step")`
- Multiple buttons can trigger the same action
- If you want different behaviors, use different mapping scripts with unique `source` names
- Currently only `"play_step"` is handled; other sources can be added in the sequencer code

## Advanced Customization

### Adding Custom Sequencer Actions

To add new button actions (beyond "play step"), modify the WebSocket handler in `web/sequencer.html`:

```javascript
if (message.type === 'seq_event') {
  switch(message.source) {
    case 'play_step':
      if (message.value > 0) stepPlay();
      break;
    case 'reset':
      if (message.value > 0) resetPlay();
      break;
    // Add more actions here
  }
}
```

## Files Modified

- `web/index.html` - Added mapping template dropdown
- `web/js/component-config.js` - Added `initMappingTemplate()` function
- `web/sequencer.html` - Added `initWebSocket()` function for button events
- `src/mapping/MappingEngine.cpp` - Already supports `seq.out()` function

## Summary

You now have a complete physical button-to-sequencer mapping system:

```
Physical Button Press
     ↓
GPIO Input Detected
     ↓
MappingEngine executes: seq.out("play_step")
     ↓
WebSocket sends JSON: {"type":"seq_event","source":"play_step","value":1}
     ↓
Sequencer.html receives and processes
     ↓
stepPlay() function called
     ↓
Next step in NIDMI file plays
```

Enjoy controlling your sequencer with physical buttons!
