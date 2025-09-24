# Architecture MIDI Optimisée - ESP32Server

## Table des matières
1. [Architecture Template](#architecture-template)
2. [Filtrage Analogique](#filtrage-analogique)
3. [Anti-rebond (Debouncing)](#anti-rebond-debouncing)
4. [Multiplexeurs](#multiplexeurs)
5. [Interface Web Dynamique](#interface-web-dynamique)
6. [Exemples Pédagogiques](#exemples-pédagogiques)
7. [Optimisation Mémoire](#optimisation-mémoire)

---

## Architecture Template

### Problème : Mémoire limitée (15ko)
Les classes séparées consomment trop de mémoire :
```cpp
// ❌ Approche classique (gourmande)
class MidiNote { uint8_t note, velocity, channel; };     // 12 bytes
class MidiCC { uint8_t cc, value, channel; };          // 12 bytes  
class MidiPC { uint8_t program, channel; };             // 8 bytes
// Total: 32 bytes par type
```

### Solution : Template optimisé
```cpp
// ✅ Approche template (optimisée)
enum MidiType { NOTE, CC, PC, PITCH_BEND, AFTERTOUCH, CLOCK };

template<MidiType T>
class MidiMessage {
    union {
        struct { uint8_t note, velocity, channel; } note;
        struct { uint8_t cc, value, channel; } cc;
        struct { uint8_t program, channel; } pc;
        struct { uint8_t channel; } pitch_bend;
        struct { uint8_t channel; } aftertouch;
        struct { uint8_t dummy; } clock;
    } data;
    
    void send() {
        switch(T) {
            case NOTE: 
                MIDI.sendNoteOn(data.note.note, data.note.velocity, data.note.channel);
                break;
            case CC: 
                MIDI.sendControlChange(data.cc.cc, data.cc.value, data.cc.channel);
                break;
            case PC: 
                MIDI.sendProgramChange(data.pc.program, data.pc.channel);
                break;
        }
    }
};
// Total: 8 bytes (union + type)
```

### Gain mémoire : 75% de réduction !

---

## Filtrage Analogique

### Problème : Bruit sur les potentiomètres
Les potentiomètres génèrent du bruit qui cause des messages MIDI parasites.

### Solution : Filtre numérique adaptatif
```cpp
class AnalogFilter {
private:
    float alpha;           // Coefficient de filtrage (0.0-1.0)
    float last_value;      // Valeur précédente
    uint16_t threshold;    // Seuil de changement
    
public:
    AnalogFilter(float alpha = 0.1, uint16_t threshold = 5) 
        : alpha(alpha), last_value(0), threshold(threshold) {}
    
    uint16_t filter(uint16_t raw_value) {
        // Filtre passe-bas exponentiel
        float filtered = alpha * raw_value + (1.0 - alpha) * last_value;
        last_value = filtered;
        
        // Seuil de changement pour éviter les micro-variations
        if(abs(filtered - last_value) < threshold) {
            return last_value;
        }
        
        return (uint16_t)filtered;
    }
    
    // Adaptation automatique du coefficient selon la vitesse de changement
    void adaptFilter(uint16_t current_value) {
        float change_rate = abs(current_value - last_value);
        
        if(change_rate > 50) {
            alpha = 0.3;  // Changement rapide : filtre moins agressif
        } else if(change_rate < 10) {
            alpha = 0.05; // Changement lent : filtre plus agressif
        } else {
            alpha = 0.1;  // Valeur par défaut
        }
    }
};
```

### Utilisation avec potentiomètre
```cpp
class Potentiometer {
private:
    uint8_t pin;
    AnalogFilter filter;
    MidiMessage<CC> midi_msg;
    uint16_t last_sent_value;
    
public:
    Potentiometer(uint8_t pin, uint8_t cc, uint8_t channel) 
        : pin(pin), midi_msg(), last_sent_value(0) {
        midi_msg.data.cc.cc = cc;
        midi_msg.data.cc.channel = channel;
    }
    
    void update() {
        uint16_t raw_value = analogRead(pin);
        filter.adaptFilter(raw_value);
        uint16_t filtered_value = filter.filter(raw_value);
        
        // Conversion 0-4095 → 0-127
        uint8_t midi_value = map(filtered_value, 0, 4095, 0, 127);
        
        // Envoyer seulement si changement significatif
        if(abs(midi_value - last_sent_value) >= 2) {
            midi_msg.data.cc.value = midi_value;
            midi_msg.send();
            last_sent_value = midi_value;
        }
    }
};
```

---

## Anti-rebond (Debouncing)

### Problème : Rebonds sur les boutons
Les boutons mécaniques génèrent plusieurs impulsions pour une seule pression.

### Solution : Debouncing intelligent
```cpp
class ButtonDebounce {
private:
    uint8_t pin;
    bool last_state;
    uint32_t last_press_time;
    uint32_t debounce_delay;
    bool press_detected;
    
public:
    ButtonDebounce(uint8_t pin, uint32_t debounce_delay = 50) 
        : pin(pin), last_state(false), last_press_time(0), 
          debounce_delay(debounce_delay), press_detected(false) {
        pinMode(pin, INPUT_PULLUP);
    }
    
    bool isPressed() {
        bool current_state = !digitalRead(pin);  // Inversé car PULLUP
        uint32_t now = millis();
        
        // Détection de pression (front montant)
        if(current_state && !last_state) {
            if(now - last_press_time > debounce_delay) {
                last_press_time = now;
                press_detected = true;
                return true;
            }
        }
        
        // Détection de relâchement (front descendant)
        if(!current_state && last_state) {
            if(now - last_press_time > debounce_delay) {
                last_press_time = now;
                press_detected = false;
                return false;
            }
        }
        
        last_state = current_state;
        return false;
    }
    
    bool isReleased() {
        return !isPressed() && press_detected;
    }
};
```

### Utilisation avec bouton MIDI
```cpp
class Button {
private:
    ButtonDebounce debouncer;
    MidiMessage<NOTE> midi_msg;
    bool note_on_sent;
    
public:
    Button(uint8_t pin, uint8_t note, uint8_t channel) 
        : debouncer(pin), note_on_sent(false) {
        midi_msg.data.note.note = note;
        midi_msg.data.note.channel = channel;
    }
    
    void update() {
        if(debouncer.isPressed() && !note_on_sent) {
            // Note On
            midi_msg.data.note.velocity = 127;
            midi_msg.send();
            note_on_sent = true;
            
        } else if(debouncer.isReleased() && note_on_sent) {
            // Note Off
            midi_msg.data.note.velocity = 0;
            midi_msg.send();
            note_on_sent = false;
        }
    }
};
```

---

## Multiplexeurs

### Multiplexeur 16:1 (CD4067)
```cpp
class Multiplexer16 {
private:
    uint8_t select_pins[4];  // S0, S1, S2, S3
    uint8_t analog_pin;
    
public:
    Multiplexer16(uint8_t s0, uint8_t s1, uint8_t s2, uint8_t s3, uint8_t analog_pin) 
        : analog_pin(analog_pin) {
        select_pins[0] = s0;
        select_pins[1] = s1;
        select_pins[2] = s2;
        select_pins[3] = s3;
        
        for(int i = 0; i < 4; i++) {
            pinMode(select_pins[i], OUTPUT);
        }
    }
    
    uint16_t read(uint8_t channel) {
        // Sélectionner le canal (0-15)
        for(int i = 0; i < 4; i++) {
            digitalWrite(select_pins[i], (channel >> i) & 1);
        }
        
        // Petit délai pour stabilisation
        delayMicroseconds(10);
        
        return analogRead(analog_pin);
    }
};
```

### Potentiomètre avec multiplexeur
```cpp
class PotentiometerMux {
private:
    Multiplexer16* mux;
    uint8_t channel;
    AnalogFilter filter;
    MidiMessage<CC> midi_msg;
    uint16_t last_sent_value;
    
public:
    PotentiometerMux(Multiplexer16* mux, uint8_t channel, uint8_t cc, uint8_t midi_channel) 
        : mux(mux), channel(channel), last_sent_value(0) {
        midi_msg.data.cc.cc = cc;
        midi_msg.data.cc.channel = midi_channel;
    }
    
    void update() {
        uint16_t raw_value = mux->read(channel);
        filter.adaptFilter(raw_value);
        uint16_t filtered_value = filter.filter(raw_value);
        
        uint8_t midi_value = map(filtered_value, 0, 4095, 0, 127);
        
        if(abs(midi_value - last_sent_value) >= 2) {
            midi_msg.data.cc.value = midi_value;
            midi_msg.send();
            last_sent_value = midi_value;
        }
    }
};
```

---

## Interface Web Dynamique

### Configuration des multiplexeurs
```html
<!-- Sélection du type de pin -->
<div class="pin-config">
    <label>Type de pin:</label>
    <select id="pinType" onchange="updateMuxConfig()">
        <option value="direct">Pin direct</option>
        <option value="mux16">Multiplexeur 16:1</option>
        <option value="mux8">Multiplexeur 8:1</option>
    </select>
    
    <!-- Configuration multiplexeur -->
    <div id="muxConfig" style="display:none">
        <h4>Configuration Multiplexeur</h4>
        
        <div class="form-group">
            <label>Pins de sélection:</label>
            <input type="number" id="muxS0" placeholder="S0" min="0" max="21">
            <input type="number" id="muxS1" placeholder="S1" min="0" max="21">
            <input type="number" id="muxS2" placeholder="S2" min="0" max="21">
            <input type="number" id="muxS3" placeholder="S3" min="0" max="21">
        </div>
        
        <div class="form-group">
            <label>Pin analogique:</label>
            <input type="number" id="muxAnalog" placeholder="A0" min="0" max="21">
        </div>
        
        <div class="form-group">
            <label>Canal (0-15):</label>
            <input type="number" id="muxChannel" min="0" max="15" value="0">
        </div>
    </div>
    
    <!-- Configuration MIDI -->
    <div class="form-group">
        <label>Type MIDI:</label>
        <select id="midiType">
            <option value="cc">Control Change</option>
            <option value="note">Note On/Off</option>
            <option value="pc">Program Change</option>
        </select>
    </div>
    
    <div class="form-group">
        <label>CC/Note Number:</label>
        <input type="number" id="midiNumber" min="0" max="127" value="1">
    </div>
    
    <div class="form-group">
        <label>Channel MIDI:</label>
        <input type="number" id="midiChannel" min="1" max="16" value="1">
    </div>
    
    <button onclick="savePinConfig()">Sauvegarder</button>
</div>
```

### JavaScript dynamique
```javascript
// Gestion des multiplexeurs
function updateMuxConfig() {
    const pinType = document.getElementById('pinType').value;
    const muxConfig = document.getElementById('muxConfig');
    
    if(pinType === 'mux16' || pinType === 'mux8') {
        muxConfig.style.display = 'block';
        
        // Afficher seulement S0-S2 pour mux8
        if(pinType === 'mux8') {
            document.getElementById('muxS3').style.display = 'none';
        } else {
            document.getElementById('muxS3').style.display = 'inline';
        }
    } else {
        muxConfig.style.display = 'none';
    }
}

// Sauvegarde de la configuration
async function savePinConfig() {
    const config = {
        pin: document.getElementById('pinNumber').value,
        type: document.getElementById('pinType').value,
        mux_s0: document.getElementById('muxS0').value,
        mux_s1: document.getElementById('muxS1').value,
        mux_s2: document.getElementById('muxS2').value,
        mux_s3: document.getElementById('muxS3').value,
        mux_analog: document.getElementById('muxAnalog').value,
        mux_channel: document.getElementById('muxChannel').value,
        midi_type: document.getElementById('midiType').value,
        midi_number: document.getElementById('midiNumber').value,
        midi_channel: document.getElementById('midiChannel').value
    };
    
    try {
        const response = await fetch('/api/pins/set', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify(config)
        });
        
        if(response.ok) {
            showMessage('Configuration sauvegardée !', 'success');
        } else {
            showMessage('Erreur lors de la sauvegarde', 'error');
        }
    } catch(error) {
        showMessage('Erreur réseau: ' + error.message, 'error');
    }
}
```

---

## Exemples Pédagogiques

### Niveau 1 : Premier Potentiomètre
```cpp
void setup() {
    // 1 pin → 1 CC
    Potentiometer pot(A0, 1, 7);  // Pin A0, CC 7, Channel 1
}

void loop() {
    pot.update();  // Envoie CC 7 automatiquement
}
```

### Niveau 2 : Multiplexeur
```cpp
void setup() {
    // 1 pin → 16 potentiomètres
    Multiplexer16 mux(2, 3, 4, 5, A0);  // S0-S3, analog
    
    for(int i = 0; i < 16; i++) {
        pots[i].begin(&mux, i, i+1, 1);  // Canal i, CC i+1
    }
}
```

### Niveau 3 : Projet Final
```cpp
// Synthesizer avec 16 potentiomètres
class Synthesizer {
private:
    Multiplexer16 mux;
    PotentiometerMux pots[16];
    
public:
    void begin() {
        mux.begin(2, 3, 4, 5, A0);
        
        // Configuration des paramètres
        pots[0].begin(&mux, 0, 1, 1);   // Fréquence
        pots[1].begin(&mux, 1, 2, 1);   // Filtre
        pots[2].begin(&mux, 2, 3, 1);   // LFO
        pots[3].begin(&mux, 3, 7, 1);   // Volume
        // ... etc
    }
    
    void update() {
        for(int i = 0; i < 16; i++) {
            pots[i].update();
        }
    }
};
```

---

## Optimisation Mémoire

### Capacité avec 15ko disponibles
- **64 potentiomètres** (4 × 16) = **512 bytes**
- **64 boutons** (4 × 16) = **512 bytes**
- **32 LEDs** (2 × 16) = **256 bytes**
- **Total** : **1.3ko** pour 160 composants !

### Configuration dynamique
```cpp
// Structure de configuration optimisée
struct PinConfig {
    uint8_t pin;
    uint8_t type;        // POT, BUTTON, LED
    uint8_t mux_channel; // 0-15 pour multiplexeur
    uint8_t midi_cc;     // CC number
    uint8_t midi_channel;
};

// Tableau de configuration (160 composants max)
PinConfig configs[160];
```

### Exemple complet
```cpp
void setup() {
    // 4 multiplexeurs 16:1
    Multiplexer16 mux1(2, 3, 4, 5, A0);
    Multiplexer16 mux2(6, 7, 8, 9, A1);
    Multiplexer16 mux3(10, 11, 12, 13, A2);
    Multiplexer16 mux4(14, 15, 16, 17, A3);
    
    // 64 potentiomètres
    PotentiometerMux pots[64];
    
    for(int i = 0; i < 64; i++) {
        Multiplexer16* mux = &mux1;
        if(i >= 16) mux = &mux2;
        if(i >= 32) mux = &mux3;
        if(i >= 48) mux = &mux4;
        
        pots[i].begin(mux, i % 16, i + 1, 1);
    }
}

void loop() {
    for(int i = 0; i < 64; i++) {
        pots[i].update();
    }
}
```

---

## Conclusion

Cette architecture permet de créer des contrôleurs MIDI professionnels avec :
- ✅ **Interface web intuitive**
- ✅ **Configuration en temps réel**
- ✅ **Filtrage analogique intelligent**
- ✅ **Anti-rebond robuste**
- ✅ **Support des multiplexeurs**
- ✅ **Mémoire optimisée (75% de réduction)**
- ✅ **Projets pédagogiques progressifs**

**Parfait pour l'éducation et les projets créatifs !** 🎵🚀
