/*
 * Exemple ESP32Server avec BLE MIDI
 * 
 * Ce sketch démontre l'utilisation du BLE MIDI avec ESP32Server.
 * 
 * Pour activer BLE MIDI, ajoutez cette ligne au début du fichier :
 * #define ESP32SERVER_ENABLE_BLE_MIDI
 * 
 * Fonctionnalités :
 * - BLE MIDI automatiquement activé
 * - Boutons et potentiomètres routés vers BLE
 * - Interface web pour configuration
 * 
 * Connexion :
 * - Rechercher "ESP32-MIDI" dans les paramètres Bluetooth de votre appareil
 * - Se connecter (pas de code PIN requis)
 * - Utiliser avec des apps MIDI comme GarageBand, Ableton Live, etc.
 * 
 * Matériel :
 * - ESP32-C3 ou ESP32-S3
 * - Boutons sur GPIO 2, 3, 4, 5
 * - Potentiomètres sur GPIO 6, 7, 8, 9
 * - LEDs sur GPIO 10, 11, 12, 13
 */

// Activer BLE MIDI pour cet exemple
#define ESP32SERVER_ENABLE_BLE_MIDI

#include <esp32server.h>

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== ESP32Server BLE MIDI ===");
    
    // Initialiser ESP32Server (BLE activé automatiquement)
    esp32server_setup();
    
    // Configuration des composants
    // Boutons (Note On/Off)
    esp32server_addButton(2, 1, 60, 1);  // GPIO 2 -> Note C4, Channel 1
    esp32server_addButton(3, 1, 62, 1);  // GPIO 3 -> Note D4, Channel 1
    esp32server_addButton(4, 1, 64, 1);  // GPIO 4 -> Note E4, Channel 1
    esp32server_addButton(5, 1, 65, 1);   // GPIO 5 -> Note F4, Channel 1
    
    // Potentiomètres (Control Change)
    esp32server_addPotentiometer(6, 1, 1, 1);   // GPIO 6 -> CC1, Channel 1
    esp32server_addPotentiometer(7, 1, 2, 1);    // GPIO 7 -> CC2, Channel 1
    esp32server_addPotentiometer(8, 1, 3, 1);    // GPIO 8 -> CC3, Channel 1
    esp32server_addPotentiometer(9, 1, 4, 1);    // GPIO 9 -> CC4, Channel 1
    
    // LEDs (contrôlées par MIDI entrant)
    esp32server_addLed(10, 1, 60, 1);  // LED 1 -> Note C4
    esp32server_addLed(11, 1, 62, 1);  // LED 2 -> Note D4
    esp32server_addLed(12, 1, 64, 1);  // LED 3 -> Note E4
    esp32server_addLed(13, 1, 65, 1);  // LED 4 -> Note F4
    
    Serial.println("Configuration terminée !");
    Serial.println("Recherchez 'ESP32-MIDI' dans vos paramètres Bluetooth");
    Serial.println("Interface web disponible sur http://esp32server.local");
}

void loop() {
    // Boucle principale ESP32Server
    esp32server_loop();
    
    // Votre code personnalisé ici
    // Le BLE MIDI est géré automatiquement
}
