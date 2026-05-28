/*
 * Pont USB <-> UART pour debugger un autre ESP — typiquement un NiDMI en mode
 * USB-MIDI (ON), où la console USB-CDC n'existe pas (cdc_on_boot=0) et où le WiFi
 * peut être tombé : on n'a alors AUCUNE observabilité par l'USB du NiDMI.
 *
 * Astuce clé : en mode ON, `Serial` du NiDMI = UART0 matériel = broche TX
 * (GPIO43 = pad D6 sur XIAO ESP32-S3). Les logs sortent donc DÉJÀ sur D6,
 * sans rien changer au firmware. Ce pont les lit et les renvoie sur SON USB.
 *
 * Câblage (lecture seule = suffisant pour diagnostiquer) :
 *   NiDMI D6 (GPIO43, UART0 TX)  ->  BRIDGE_RX_PIN de CE pont
 *   NiDMI GND                    ->  GND de CE pont
 *   (optionnel, pour aussi ENVOYER au NiDMI :
 *     BRIDGE_TX_PIN de ce pont   ->  NiDMI D7 (GPIO44, UART0 RX))
 *
 * Les deux cartes sont en 3.3V : liaison directe, pas de level shifter.
 *
 * Adapter BRIDGE_RX_PIN / BRIDGE_TX_PIN à des GPIO LIBRES de TA carte pont,
 * flasher ce sketch (build par défaut), puis ouvrir le port USB du pont à
 * 115200 (ex. `python3 scripts/serial_monitor.py`).
 */

// GPIO de CE pont reliés au NiDMI — à adapter selon ta carte pont :
#define BRIDGE_RX_PIN 4    // relié au TX du NiDMI (D6 / GPIO43)
#define BRIDGE_TX_PIN 5    // relié au RX du NiDMI (D7 / GPIO44) — optionnel
#define UART_BAUD     115200

void setup() {
  Serial.begin(UART_BAUD);                                              // USB -> ordinateur
  Serial1.begin(UART_BAUD, SERIAL_8N1, BRIDGE_RX_PIN, BRIDGE_TX_PIN);   // UART <-> NiDMI
  delay(200);
  Serial.print("[uart-bridge] pret @");
  Serial.print(UART_BAUD);
  Serial.print(" RX=");
  Serial.println(BRIDGE_RX_PIN);
}

void loop() {
  while (Serial1.available()) Serial.write(Serial1.read());  // NiDMI -> PC
  while (Serial.available())  Serial1.write(Serial.read());  // PC -> NiDMI (optionnel)
}
