/**
 * Démo serveur web ESP32Server (RTP‑MIDI prêt), configuration via l’interface web.
 *
 * Ce sketch démarre uniquement la librairie:
 * - AP + mDNS + serveur web (nom lu depuis NVS, fallback "esp32rtpmidi")
 * - Connexion STA si déjà configurée côté UI
 * - RTP‑MIDI activé automatiquement si STA est connecté
 *
 * Toute la configuration (nom, STA, etc.) se fait depuis la page web.
 * Aucun autre code utilisateur n’est nécessaire.
 */

#include <Esp32Server.h>
#include <WiFi.h>

void setup(){
	Serial.begin(115200);
	delay(200);
	Serial.println();
	Serial.println(F("[Exemple] Ouvrez la console série à 115200 bauds."));
	Serial.println(F("[Exemple] En mode AP, utilisez l'adresse: http://192.168.4.1/"));
	Serial.println(F("[Exemple] Remarque: mDNS (.local) fonctionne en mode STA (connecté à votre Wi‑Fi)."));

	esp32server.begin();

	// Affiche l'IP AP après le démarrage
	Serial.print(F("[Exemple] AP IP: "));
	Serial.println(WiFi.softAPIP());
}

void loop(){
	esp32server.loop();
}


