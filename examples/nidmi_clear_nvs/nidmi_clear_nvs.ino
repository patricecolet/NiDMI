/*
 * NiDMI - Clear NVS
 * 
 * Sketch simple pour nettoyer complètement la NVS (Non-Volatile Storage)
 * 
 * Usage :
 * 1. Téléverser ce sketch sur l'ESP32
 * 2. Le sketch nettoie la NVS et redémarre
 * 3. Téléverser ensuite nidmi_basic.ino pour repartir avec une configuration vierge
 * 
 * Ce sketch supprime toutes les configurations stockées en NVS :
 * - Configurations des pins
 * - Configuration WiFi STA
 * - Configuration mDNS
 * - Configuration OSC
 * - Toutes les autres données stockées
 */

#include <Preferences.h>

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println();
    Serial.println("========================================");
    Serial.println("  NiDMI - Clear NVS");
    Serial.println("========================================");
    Serial.println();
    
    // Nettoyer la NVS
    Preferences preferences;
    preferences.begin("nidmi", false);
    
    Serial.println("[Clear NVS] Suppression de toutes les données NVS...");
    bool cleared = preferences.clear();
    preferences.end();
    
    if (cleared) {
        Serial.println("[Clear NVS] ✅ NVS nettoyée avec succès !");
    } else {
        Serial.println("[Clear NVS] ❌ Erreur lors du nettoyage de la NVS");
    }
    
    Serial.println();
    Serial.println("NVS réinitialisée. Vous pouvez maintenant téléverser nidmi_basic.ino");
    Serial.println();
    
    // Attendre un peu avant de redémarrer
    delay(2000);
    
    // Redémarrer l'ESP32
    Serial.println("[Clear NVS] Redémarrage dans 3 secondes...");
    delay(3000);
    ESP.restart();
}

void loop() {
    // Rien à faire ici, le sketch redémarre dans setup()
}
