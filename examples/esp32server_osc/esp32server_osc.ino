/**
 * @file esp32server_osc.ino
 * @brief Exemple d'utilisation de l'ESP32Server avec communication OSC
 * @author ESP32Server Library
 * @date 2024
 * 
 * @details
 * Ce sketch démontre l'utilisation de l'ESP32Server avec un focus sur
 * la communication OSC (Open Sound Control). Il configure l'ESP32 comme
 * un serveur web avec interface de configuration et envoie des messages
 * OSC vers des destinations configurées.
 * 
 * @section features Fonctionnalités
 * - Serveur web avec interface de configuration
 * - Communication OSC unicast et broadcast
 * - Configuration des pins avec RTP-MIDI et OSC
 * - Support des modes AP et STA
 * - Interface web responsive
 * 
 * @section osc_messages Messages OSC
 * - /note : Envoi de notes MIDI (note, vélocité, canal)
 * - /ctl : Envoi de contrôleurs MIDI (CC, valeur, canal)
 * - /led : Contrôle des LEDs (on/off, PWM)
 * 
 * @section network Configuration réseau
 * - Mode AP : 192.168.4.1 (SSID: ESP32Server)
 * - Mode STA : Connexion à un réseau Wi-Fi existant
 * - mDNS : http://esp32server.local
 * 
 * @section usage Utilisation
 * 1. Téléversez ce sketch sur votre ESP32
 * 2. Connectez-vous au réseau ESP32Server (mode AP)
 * 3. Ouvrez http://192.168.4.1 dans votre navigateur
 * 4. Configurez les pins et les destinations OSC
 * 5. Testez avec le patch Pure Data fourni
 * 
 * @section examples Exemples de messages OSC
 * @code
 * // Note MIDI
 * /note 60 100 1    // Note 60, vélocité 100, canal 1
 * 
 * // Contrôleur MIDI
 * /ctl 7 64 1       // CC#7, valeur 64, canal 1
 * 
 * // LED
 * /led 1            // LED allumée
 * /led 0            // LED éteinte
 * @endcode
 * 
 * @section dependencies Dépendances
 * - ESP32Server Library
 * - WiFi (ESP32)
 * - Preferences (ESP32)
 * - ESPAsyncWebServer
 * - OSC Library
 * - AppleMIDI Library
 * 
 * @section hardware Matériel requis
 * - ESP32-C3 ou ESP32-S3
 * - Câble USB pour programmation
 * - Composants optionnels : potentiomètres, boutons, LEDs
 * 
 * @section troubleshooting Dépannage
 * - Vérifiez la connexion Wi-Fi
 * - Assurez-vous que le port OSC (8000) est libre
 * - Vérifiez les logs série pour les erreurs
 * - Testez avec le patch Pure Data fourni
 * 
 * @see esp32server_basic.ino Pour un exemple plus simple
 * @see esp32server_debug.ino Pour le debug avancé
 * @see esp32_receiver.pd Patch Pure Data de test
 */

// ============================================================================
// INCLUDE DE LA BIBLIOTHÈQUE PRINCIPALE
// ============================================================================

#include <Esp32Server.h>

// ============================================================================
// CONFIGURATION GÉNÉRALE
// ============================================================================

/**
 * @brief Nom du serveur mDNS
 * @details Ce nom sera utilisé pour l'AP Wi-Fi et http://nom.local
 */
const char* SERVER_NAME = "esp32server";

/**
 * @brief Port OSC par défaut
 * @details Port utilisé pour la communication OSC
 */
const int OSC_PORT = 8000;

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

/**
 * @brief Instance principale du serveur ESP32
 * @details Gère le serveur web, les WebSockets et la communication OSC
 */
Esp32Server esp32server;

// ============================================================================
// FONCTIONS UTILITAIRES
// ============================================================================

/**
 * @brief Initialise la communication série
 * @details Configure le port série pour le debug et les logs
 */
void setupSerial() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== ESP32Server OSC Example ===");
    Serial.println("Configuration OSC avec interface web");
    Serial.println("=====================================\n");
}

/**
 * @brief Affiche les informations de connexion
 * @details Affiche l'IP et le nom mDNS du serveur
 */
void printConnectionInfo() {
    Serial.println("🌐 Serveur démarré !");
    Serial.print("📱 Interface web: http://");
    Serial.println(WiFi.localIP());
    Serial.print("🔗 mDNS: http://");
    Serial.print(SERVER_NAME);
    Serial.println(".local");
    Serial.print("🎵 OSC Port: ");
    Serial.println(OSC_PORT);
    Serial.println("📡 Mode: Broadcast OSC activé");
    Serial.println("=====================================\n");
}

// ============================================================================
// SETUP PRINCIPAL
// ============================================================================

/**
 * @brief Fonction d'initialisation principale
 * @details Configure et démarre tous les services
 */
void setup() {
    // Initialisation de la communication série
    setupSerial();
    
    // Configuration du serveur ESP32
    esp32server.begin();
    
    // Configuration du nom mDNS
    esp32server.setMdnsName(SERVER_NAME);
    
    // Configuration OSC par défaut
    esp32server.setOscPort(OSC_PORT);
    esp32server.setOscBroadcast(true);
    
    // Affichage des informations de connexion
    printConnectionInfo();
    
    Serial.println("✅ Serveur prêt !");
    Serial.println("🌐 Connectez-vous à l'interface web pour configurer");
    Serial.println("🎵 Utilisez le patch Pure Data pour tester OSC");
    Serial.println("📱 Ou connectez-vous via http://esp32server.local");
}

// ============================================================================
// BOUCLE PRINCIPALE
// ============================================================================

/**
 * @brief Boucle principale du programme
 * @details Gère les tâches en arrière-plan et la communication
 */
void loop() {
    // La bibliothèque ESP32Server gère tout automatiquement
    // Pas besoin de code supplémentaire ici
    
    // Petite pause pour éviter de surcharger le CPU
    delay(10);
}

// ============================================================================
// NOTES D'UTILISATION
// ============================================================================

/**
 * @page osc_usage Guide d'utilisation OSC
 * 
 * @section configuration Configuration initiale
 * 1. Téléversez ce sketch sur votre ESP32
 * 2. Ouvrez le moniteur série (115200 baud)
 * 3. Connectez-vous au réseau "ESP32Server"
 * 4. Ouvrez http://192.168.4.1 dans votre navigateur
 * 
 * @section pins Configuration des pins
 * - Cliquez sur les pins du schéma pour les configurer
 * - Choisissez le type : Potentiomètre, Bouton, LED, etc.
 * - Configurez RTP-MIDI et OSC selon vos besoins
 * - Sauvegardez avec "Enregistrer tout"
 * 
 * @section osc_test Test OSC
 * - Utilisez le patch Pure Data fourni (esp32_receiver.pd)
 * - Ouvrez le patch dans Pure Data
 * - Les messages OSC seront reçus automatiquement
 * - Testez avec les contrôles de l'interface web
 * 
 * @section network_modes Modes réseau
 * - **Mode AP** : ESP32 crée son propre réseau
 * - **Mode STA** : ESP32 se connecte à un réseau existant
 * - **Mode hybride** : Les deux modes simultanément
 * 
 * @section osc_destinations Destinations OSC
 * - **IP spécifique** : Envoi vers une adresse IP précise
 * - **Broadcast AP** : Diffusion vers 192.168.4.255
 * - **Broadcast STA** : Diffusion vers le réseau connecté
 * 
 * @section troubleshooting Dépannage
 * - Vérifiez que le port 8000 est libre
 * - Assurez-vous que le firewall n'bloque pas UDP
 * - Testez d'abord en mode broadcast
 * - Vérifiez les logs série pour les erreurs
 */
