# Guide avancé (MIDI / OSC / Temps réel)

Ce document s’adresse aux utilisateurs avancés (temps réel, intégration MIDI/OSC, différences ESP32‑C3 vs ESP32‑S3).

## Architecture et temps réel
- ESP32‑C3: 1 cœur; ESP32‑S3: 2 cœurs. Pour le temps réel, éviter de bloquer la boucle principale et toute opération réseau ou FS dans les callbacks critiques.
- Utiliser des files (queues) pour décorréler lecture capteurs/boutons (ISR ou polling rapide) de l’envoi réseau (WebSocket/OSC/MIDI) plus lent.
- Préférer des callbacks courts (ISR) qui postent des événements (structs) dans une queue consommée dans `loop()`.

Exemple (pseudo‑code) pour un bouton poussoir antirebond:
```cpp
struct Event { uint32_t t; uint8_t type; uint8_t value; };
QueueHandle_t q;

void setup(){
  q = xQueueCreate(16, sizeof(Event));
  pinMode(BTN, INPUT_PULLUP);
}

void loop(){
  static uint32_t last=0; uint32_t now=millis();
  // Polling bouton (faible coût), antirebond logiciel
  static bool prev = digitalRead(BTN);
  bool cur = digitalRead(BTN);
  if(cur!=prev && now-last>8){ // 8 ms debounce
    last = now; prev = cur;
    Event e{now, 1 /*BTN*/, (uint8_t)!cur};
    xQueueSend(q, &e, 0);
  }

  Event e;
  while(xQueueReceive(q, &e, 0)==pdTRUE){
    // Mapper -> MIDI/OSC (voir sections ci‑dessous)
  }
}
```

## Envoi OSC
- Utiliser une lib OSC Arduino (ex. `OSCMessage` d’CNMAT) ou un émetteur UDP minimal.
- Pour éviter le jitter, sérialiser sur une queue et envoyer par rafales (batch) à 250–500 Hz max.

Pseudo‑code UDP OSC minimal:
```cpp
#include <WiFiUdp.h>
WiFiUDP udp;
IPAddress dst(192,168,1,10); uint16_t port = 9000;

void sendOscButton(bool pressed){
  // Framing OSC minimal: "/btn" + int32
  const char *addr = "/btn"; // alignements OSC non garantis ici (exemple simplifié)
  uint8_t buf[32]; size_t n=0;
  // … packer proprement selon OSC (ou utiliser une lib)
  udp.beginPacket(dst, port);
  udp.write((const uint8_t*)"/btn\0\0\0", 8);
  udp.write((const uint8_t*)",i\0\0", 4);
  uint32_t v = htonl(pressed?1:0);
  udp.write((uint8_t*)&v, 4);
  udp.endPacket();
}
```

## Envoi MIDI
- MIDI DIN: via UART; WebMIDI: via WebSocket + mapping; USB‑MIDI: dépend des cartes/cores.
- MIDI sur UDP (RTP‑MIDI): plus complexe; on peut commencer par UDP simple avec un format maison, puis évoluer.

Pseudo‑code MIDI Note On/Off (UART):
```cpp
HardwareSerial &midi = Serial1; // vérifier pins selon carte
void midiBegin(){ midi.begin(31250); }
void midiNoteOn(uint8_t ch, uint8_t note, uint8_t vel){
  midi.write(0x90 | ((ch-1)&0x0F)); midi.write(note); midi.write(vel);
}
void midiNoteOff(uint8_t ch, uint8_t note, uint8_t vel){
  midi.write(0x80 | ((ch-1)&0x0F)); midi.write(note); midi.write(vel);
}
```

## C3 vs S3: répartition des tâches
- C3 (1 cœur): privilégier le modèle queue + boucle; éviter Async lourds dans `loop()`; attention aux sections critiques.
- S3 (2 cœurs): on peut épingler les tâches (FreeRTOS) sur `APP_CPU` pour le temps réel et laisser le Wi‑Fi/serveur sur l’autre cœur. Ne pas sur‑complexifier au départ.

## Hooks à venir dans Esp32Server
- Ajout d’un canal d’événements (queue) exposé par la librairie pour poster des “input events” (bouton, pot, encoder) côté user code.
- Callbacks d’abstraction (ex: `onButton(bool pressed)`) avec cadence contrôlée.
- Routage interne vers WebSocket et/ou OSC/MIDI en parallèle, activables par options.

## Bonnes pratiques temps réel
- Pas de malloc/free dans la boucle audio/critique; allouer statiquement ou au setup.
- Limiter la fréquence d’envoi réseau; regrouper les messages.
- Antirebond logiciel maîtrisé (5–10 ms) ou matériel.
- Mesurer (Serial log à faible cadence, `millis()`/`micros()`), puis ajuster.

## Exemples à venir
- Bouton → Note On/Off MIDI
- Bouton → OSC /btn 1|0
- Encoder → CC MIDI / OSC
- Potentiomètre → CC (lin/log) avec seuils de delta
