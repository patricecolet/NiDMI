/*
 * Exemple RTP-MIDI basique pour ESP32
 * 
 * Ce sketch démontre l'utilisation du serveur web ESP32 avec support RTP-MIDI.
 * 
 * Fonctionnalités :
 * - Point d'accès Wi-Fi : "ESP32-RTP-MIDI" (mot de passe : "esp32pass")
 * - Interface web accessible via http://esp32rtpmidi.local/
 * - Configuration RTP-MIDI via l'interface web
 * - Support connexion Wi-Fi STA (optionnel)
 * 
 * Configuration RTP-MIDI :
 * 1. Connectez-vous au point d'accès "ESP32-RTP-MIDI"
 * 2. Ouvrez http://esp32rtpmidi.local/ dans votre navigateur
 * 3. Allez dans l'onglet "Connection"
 * 4. Configurez le nom RTP-MIDI et activez le service
 * 5. Votre ESP32 apparaîtra dans les périphériques MIDI de votre Mac/PC
 * 
 * Note : RTP-MIDI nécessite une connexion réseau (Wi-Fi STA recommandé)
 */

#include <Esp32Server.h>
#include <Preferences.h>

// Instance du serveur ESP32
Esp32Server srv;
Esp32Server& esp32Server = srv; // Référence globale pour l'accès depuis l'API

void setup(){
	// ========================================
	// INITIALISATION DU PORT SÉRIE
	// ========================================
	Serial.begin(115200);
	while(!Serial){ delay(10); } // Attendre que le port série soit prêt
	Serial.println("=== ESP32 RTP-MIDI Server ===");
	
	// ========================================
	// CONFIGURATION DU POINT D'ACCÈS WI-FI
	// ========================================
	const char* apSsid = "ESP32-RTP-MIDI";     // Nom du réseau Wi-Fi créé par l'ESP32
	const char* apPass = "esp32pass";           // Mot de passe du réseau
	const char* host = "esp32rtpmidi";         // Nom pour mDNS (http://esp32rtpmidi.local/)
	
	// Démarrage du serveur web avec point d'accès
	srv.begin(apSsid, apPass, host);
	
	// Affichage des informations de connexion
	Serial.print("AP SSID: "); Serial.println(apSsid);
	Serial.print("AP PASS: "); Serial.println(apPass);
	Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
	Serial.print("Interface web: http://"); Serial.print(host); Serial.println(".local/");

	// ========================================
	// CONFIGURATION DU RÉSEAU STA (CLIENT WI-FI)
	// ========================================
	// L'ESP32 peut aussi se connecter à un réseau Wi-Fi existant
	// Décommentez et modifiez selon votre réseau :
	const char* staSsid = "manca";              // Nom de votre réseau Wi-Fi
	const char* staPass = "manca2022";         // Mot de passe de votre réseau
	srv.connectSta(staSsid, staPass);          // Connexion au réseau Wi-Fi

	// ========================================
	// CHARGEMENT DE LA CONFIGURATION RTP-MIDI
	// ========================================
	// La bibliothèque Preferences permet de sauvegarder des données de manière persistante
	// dans la mémoire flash de l'ESP32 (contrairement à la RAM qui se vide au redémarrage)
	
	Preferences preferences;                    // Créer un objet pour accéder à la mémoire persistante
	
	// LECTURE des valeurs sauvegardées
	preferences.begin("esp32server", true);   // Ouvrir le fichier de configuration
	bool rtpEnabled = preferences.getBool("rtp_enabled", false);        // Lire si RTP-MIDI est activé (défaut: false)
	String rtpName = preferences.getString("rtp_name", "ESP32-Studio"); // Lire le nom RTP-MIDI (défaut: "ESP32-Studio")
	preferences.end();                         // Fermer le fichier de configuration

	// ========================================
	// FORÇAGE DE L'ACTIVATION RTP-MIDI (POUR CE SKETCH DE TEST)
	// ========================================
	// En production, utilisez l'interface web pour activer/désactiver RTP-MIDI
	// Ici on force l'activation pour simplifier les tests
	
	preferences.begin("esp32server", true);    // Ouvrir le fichier de configuration
	preferences.putBool("rtp_enabled", true);  // Forcer l'activation de RTP-MIDI
	preferences.putString("rtp_name", "ESP32-Studio"); // Définir le nom RTP-MIDI
	preferences.putString("rtp_target", "sta"); // Définir la destination (AP ou STA)
	preferences.end();                         // Fermer et sauvegarder le fichier
	
	Serial.println("RTP-MIDI activé et sauvegardé en NVS");
	
	// ========================================
	// INITIALISATION IMMÉDIATE DE RTP-MIDI
	// ========================================
	// Initialiser RTP-MIDI directement dans setup() si STA est connecté
	if(WiFi.status() == WL_CONNECTED){
		esp32Server.rtpMidi().begin("ESP32-Studio");
		Serial.println("RTP-MIDI initialisé: ESP32-Studio");
		Serial.println("Votre ESP32 devrait maintenant apparaître dans les périphériques MIDI de votre Mac/PC");
	} else {
		Serial.println("Wi-Fi STA non connecté - RTP-MIDI sera initialisé quand la connexion sera établie");
	}
}

void loop(){
	// Variables statiques (conservent leur valeur entre les appels de loop())
	static uint32_t lastDisplay = 0;    // Dernière fois qu'on a affiché les infos réseau
	static uint32_t lastNote = 0;       // Dernière fois qu'on a envoyé une note MIDI
	static bool rtpInitialized = false; // Flag pour savoir si RTP-MIDI est initialisé
	uint32_t now = millis();            // Temps actuel en millisecondes
	
	// ========================================
	// INITIALISATION RTP-MIDI SI PAS ENCORE FAIT
	// ========================================
	// RTP-MIDI peut fonctionner sur le réseau AP (point d'accès) ou STA (client Wi-Fi)
	// On vérifie la destination configurée et on initialise si possible
	
	if(!rtpInitialized){ // Si RTP-MIDI n'est pas encore initialisé
		Preferences preferences;
		preferences.begin("esp32server", true); // Ouvrir le fichier de configuration
		
		// Lire la configuration depuis la mémoire persistante
		bool rtpEnabled = preferences.getBool("rtp_enabled", false);
		String rtpName = preferences.getString("rtp_name", "ESP32-Studio");
		String rtpTarget = preferences.getString("rtp_target", "sta");
		preferences.end();
		
		if(rtpEnabled){ // Si RTP-MIDI est activé
			bool canStart = false; // Flag pour savoir si on peut démarrer RTP-MIDI
			
			if(rtpTarget == "ap"){
				// Destination AP : toujours disponible (point d'accès créé par l'ESP32)
				canStart = true;
				Serial.println("RTP-MIDI configuré pour AP (192.168.4.1)");
			} 
			else if(rtpTarget == "sta" && WiFi.status() == WL_CONNECTED){
				// Destination STA : seulement si connecté à un réseau Wi-Fi
				canStart = true;
				Serial.println("RTP-MIDI configuré pour STA (" + WiFi.localIP().toString() + ")");
			}
			
			if(canStart){
				// Démarrer RTP-MIDI avec le nom configuré
				esp32Server.rtpMidi().begin(rtpName);
				Serial.println("RTP-MIDI initialisé: " + rtpName);
				Serial.println("Votre ESP32 devrait maintenant apparaître dans les périphériques MIDI de votre Mac/PC");
				rtpInitialized = true; // Marquer comme initialisé
			} 
			else if(rtpTarget == "sta"){
				// STA configuré mais pas connecté au Wi-Fi
				Serial.println("RTP-MIDI configuré pour STA mais Wi-Fi STA non connecté");
			}
		}
	}
	
	// ========================================
	// AFFICHAGE PÉRIODIQUE DES INFORMATIONS RÉSEAU
	// ========================================
	// Afficher les IPs toutes les 5 secondes pour surveiller la connexion
	
	if(now - lastDisplay > 5000){ // Si 5 secondes se sont écoulées
		lastDisplay = now; // Mettre à jour le timestamp
		Serial.print("AP IP: "); Serial.print(WiFi.softAPIP());    // IP du point d'accès
		Serial.print("  STA IP: "); Serial.println(WiFi.localIP()); // IP du client Wi-Fi
		
		// Afficher le statut de la connexion Wi-Fi
		if(WiFi.status() == WL_CONNECTED){
			Serial.println("Wi-Fi STA connecté - RTP-MIDI disponible");
		} else {
			Serial.println("Wi-Fi STA non connecté - RTP-MIDI indisponible");
		}
	}
	
	// ========================================
	// TEST RTP-MIDI : ENVOI DE NOTES DE TEST
	// ========================================
	// Envoyer une note MIDI toutes les 3 secondes si RTP-MIDI est connecté
	// Ceci permet de tester que RTP-MIDI fonctionne correctement
	
	if(now - lastNote > 3000){ // Si 3 secondes se sont écoulées
		lastNote = now; // Mettre à jour le timestamp
		
		if(esp32Server.rtpMidi().isConnected()){ // Si quelqu'un est connecté en RTP-MIDI
			// Envoyer une note de test (Do4, canal 1, vélocité 100)
			esp32Server.rtpMidi().sendNoteOn(1, 60, 100);  // Note On
			Serial.println("Note envoyée via RTP-MIDI");
			delay(100); // Attendre 100ms
			esp32Server.rtpMidi().sendNoteOff(1, 60, 0);   // Note Off
		}
	}
	
	// ========================================
	// MISE À JOUR RTP-MIDI (OBLIGATOIRE)
	// ========================================
	// Cette fonction doit être appelée régulièrement pour le bon fonctionnement
	// Elle traite les connexions entrantes et maintient la communication RTP-MIDI
	
	esp32Server.rtpMidi().update();
}