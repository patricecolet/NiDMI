# Guide Avancé - Architecture Temps Réel et Optimisations

Ce document s'adresse aux développeurs avancés travaillant sur l'optimisation des performances et l'architecture temps réel de NiDMI.

## 🏗️ Architecture et temps réel

### ESP32-C3 vs ESP32-S3

- **ESP32-C3** : 1 cœur RISC-V
  - Modèle queue + boucle recommandé
  - Éviter opérations lourdes dans `loop()`
  - Attention aux sections critiques (WiFi, serveur web)

- **ESP32-S3** : 2 cœurs Xtensa
  - Possibilité d'épingler les tâches FreeRTOS sur `APP_CPU` pour temps réel
  - Laisser WiFi/serveur web sur l'autre cœur
  - Ne pas sur-complexifier au départ

### Pattern Queue + Boucle

Pour le temps réel, **découpler** la lecture des capteurs (ISR ou polling rapide) de l'envoi réseau (WebSocket/OSC/MIDI) plus lent :

```cpp
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

struct Event {
    uint32_t timestamp;
    uint8_t type;
    uint8_t value;
};

QueueHandle_t eventQueue;

void setup() {
    eventQueue = xQueueCreate(16, sizeof(Event));
    pinMode(BTN_PIN, INPUT_PULLUP);
}

void loop() {
    static uint32_t lastDebounce = 0;
    uint32_t now = millis();
    
    // Polling bouton (faible coût) avec antirebond logiciel
    static bool prevState = digitalRead(BTN_PIN);
    bool currentState = digitalRead(BTN_PIN);
    
    if (currentState != prevState && (now - lastDebounce) > 8) {  // 8 ms debounce
        lastDebounce = now;
        prevState = currentState;
        
        Event e;
        e.timestamp = now;
        e.type = 1;  // BTN
        e.value = !currentState;  // LOW = pressé (INPUT_PULLUP)
        
        // Poster dans la queue (non-bloquant)
        xQueueSend(eventQueue, &e, 0);
    }
    
    // Consommer les événements de la queue
    Event e;
    while (xQueueReceive(eventQueue, &e, 0) == pdTRUE) {
        // Mapper → MIDI/OSC (opération plus lente, découplée)
        if (e.type == 1) {  // Bouton
            midiSender->sendNoteOn(1, 60, 127);
        }
    }
}
```

## 📡 Envoi OSC

### Bibliothèque recommandée

Utiliser une lib OSC Arduino (ex. `OSCMessage` de CNMAT) ou un émetteur UDP minimal.

### Éviter le jitter

- Sérialiser les messages sur une queue
- Envoyer par rafales (batch) à 250-500 Hz max
- Ne pas bloquer dans les callbacks critiques

### Exemple minimal UDP OSC

```cpp
#include <WiFiUdp.h>

WiFiUDP udp;
IPAddress destination(192, 168, 1, 10);
uint16_t port = 9000;

void sendOSCButton(bool pressed) {
    udp.beginPacket(destination, port);
    
    // Adresse OSC: "/btn"
    udp.write((const uint8_t*)"/btn\0\0\0\0", 8);  // Alignement 4 bytes
    
    // Tags: ",i" (un entier)
    udp.write((const uint8_t*)",i\0\0", 4);  // Alignement 4 bytes
    
    // Valeur: 0 ou 1
    uint32_t value = htonl(pressed ? 1 : 0);  // Network byte order
    udp.write((uint8_t*)&value, 4);
    
    udp.endPacket();
}
```

**Note** : Pour une implémentation complète OSC, utiliser une bibliothèque dédiée qui gère correctement l'alignement et tous les types de données.

## 🎵 Envoi MIDI

### MIDI DIN (UART)

```cpp
HardwareSerial& midiSerial = Serial1;  // Vérifier pins selon carte

void midiBegin() {
    midiSerial.begin(31250);  // Baudrate MIDI standard
}

void midiNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    midiSerial.write(0x90 | ((channel - 1) & 0x0F));  // Status byte
    midiSerial.write(note & 0x7F);                     // Note
    midiSerial.write(velocity & 0x7F);                 // Velocity
}

void midiNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
    midiSerial.write(0x80 | ((channel - 1) & 0x0F));  // Status byte
    midiSerial.write(note & 0x7F);                     // Note
    midiSerial.write(velocity & 0x7F);                 // Velocity
}
```

### WebSocket MIDI (RTP-MIDI)

Dans NiDMI, RTP-MIDI est géré par la bibliothèque AppleMIDI intégrée. Utiliser `MidiSender` pour l'envoi :

```cpp
midiSender->sendNoteOn(channel, note, velocity);
midiSender->sendControlChange(channel, cc, value);
```

## 🔧 Bonnes pratiques temps réel

### 1. Pas de `malloc/free` dans la boucle critique

```cpp
// ❌ MAUVAIS
void loop() {
    int* buffer = (int*)malloc(100 * sizeof(int));
    // ... utilisation ...
    free(buffer);  // Fragmentation mémoire
}

// ✅ BON
int buffer[100];  // Statique
void loop() {
    // Utiliser buffer
}
```

### 2. Limiter la fréquence d'envoi réseau

```cpp
static uint32_t lastSend = 0;
const uint32_t SEND_INTERVAL = 4;  // 250 Hz max (4 ms)

void loop() {
    uint32_t now = millis();
    if (now - lastSend < SEND_INTERVAL) return;
    lastSend = now;
    
    // Envoyer message
}
```

### 3. Regrouper les messages (batch)

```cpp
struct Message {
    uint8_t type;
    uint8_t value;
};

Message batch[16];
uint8_t batchCount = 0;

void addToBatch(uint8_t type, uint8_t value) {
    if (batchCount < 16) {
        batch[batchCount++] = {type, value};
    }
}

void sendBatch() {
    for (uint8_t i = 0; i < batchCount; i++) {
        // Envoyer batch[i]
    }
    batchCount = 0;
}
```

### 4. Antirebond logiciel maîtrisé (5-10 ms)

```cpp
class Debounce {
    uint32_t delayMs;
    uint32_t lastChange;
    bool stableState;
    bool lastRead;
    
public:
    Debounce(uint32_t delayMs = 8) 
        : delayMs(delayMs), lastChange(0), stableState(false), lastRead(false) {}
    
    bool process(bool read) {
        uint32_t now = millis();
        if (read != lastRead) {
            lastChange = now;
            lastRead = read;
        }
        if (now - lastChange >= delayMs) {
            stableState = lastRead;
        }
        return stableState;
    }
};
```

### 5. Mesurer et optimiser

```cpp
void loop() {
    static uint32_t lastLog = 0;
    uint32_t start = micros();
    
    // ... opération critique ...
    
    uint32_t duration = micros() - start;
    
    // Logger périodiquement (pas à chaque loop)
    if (millis() - lastLog > 1000) {
        Serial.printf("Duration: %lu us\n", duration);
        lastLog = millis();
    }
}
```

## 📊 Répartition des tâches (ESP32-S3)

### FreeRTOS - Épingler sur un cœur

```cpp
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void realTimeTask(void* parameter) {
    while (1) {
        // Tâche temps réel sur APP_CPU
        // Lecture capteurs, traitement audio, etc.
        vTaskDelay(1);  // 1 tick (~10ms)
    }
}

void setup() {
    // Créer tâche sur APP_CPU (cœur 1)
    xTaskCreatePinnedToCore(
        realTimeTask,
        "RealTime",
        4096,
        NULL,
        2,  // Priorité élevée
        NULL,
        1   // APP_CPU (cœur 1)
    );
    
    // WiFi et serveur web restent sur PRO_CPU (cœur 0)
}
```

**Recommandation** : Commencer simple, n'optimiser que si nécessaire.

## 🔮 Hooks à venir dans NiDMIServer

### Événements (queue) exposés

Un système d'événements pourrait être ajouté pour découpler complètement la lecture des capteurs de l'envoi réseau :

```cpp
// Interface proposée (non implémentée)
NiDMIServer::onInputEvent([](InputEvent& event) {
    // Event.type = BUTTON, POT, ENCODER, etc.
    // Event.value = valeur associée
    // Automatiquement routé vers MIDI/OSC selon config
});
```

### Callbacks d'abstraction

```cpp
// Interface proposée (non implémentée)
server.onButton(pin, [](bool pressed) {
    // Callback appelé avec cadence contrôlée
    // Anti-rebond géré automatiquement
});

server.onPotentiometer(pin, [](uint16_t value) {
    // Callback avec filtrage automatique
});
```

### Routage interne

Routage automatique vers WebSocket, OSC, et/ou MIDI en parallèle, activables par options de configuration.

## 📚 Exemples complets

### Exemple 1 : Bouton → Note On/Off MIDI

Voir `src/components/Button.h` pour implémentation complète.

### Exemple 2 : Potentiomètre → CC MIDI avec seuil

Voir `src/components/Potentiometer.h` pour implémentation avec filtrage.

### Exemple 3 : Encodeur rotatif → CC MIDI

```cpp
class EncoderRotary {
    uint8_t pinA, pinB;
    int16_t position;
    bool lastA;
    
public:
    void update() {
        bool currentA = digitalRead(pinA);
        bool currentB = digitalRead(pinB);
        
        if (currentA != lastA) {
            if (currentA == currentB) {
                position++;
            } else {
                position--;
            }
            // Envoyer CC (0-127)
            uint8_t ccValue = constrain(position, 0, 127);
            midiSender->sendControlChange(1, 7, ccValue);
        }
        lastA = currentA;
    }
};
```

## 🎯 Résumé des bonnes pratiques

1. ✅ **Queue pour découpler** lecture capteurs / envoi réseau
2. ✅ **Pas de malloc/free** dans loop critique
3. ✅ **Rate limiting** pour envoi réseau (250-500 Hz max)
4. ✅ **Anti-rebond logiciel** (5-10 ms)
5. ✅ **Mesurer avant d'optimiser** (Serial logs, micros())
6. ✅ **Batch processing** pour réduire overhead réseau
7. ✅ **FreeRTOS tasks** sur ESP32-S3 si nécessaire

---

*Guide avancé pour développeurs expérimentés*
*Basé sur l'architecture NiDMI et les meilleures pratiques ESP32*
