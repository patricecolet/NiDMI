/*
 * NiDMI - Clear NVS (effacement complet de la partition NVS)
 *
 * Usage :
 * 1. Téléverser ce sketch (mode bootloader si le port série USB ne répond plus)
 * 2. Ouvrir le moniteur série 115200 : lire le résultat puis redémarrage auto
 * 3. Téléverser nidmi_basic.ino
 *
 * Étapes :
 * - Essaie Preferences sur le namespace "nidmi" (si la NVS est encore lisible)
 * - Puis nvs_flash_deinit() + nvs_flash_erase() : efface TOUTE la partition NVS
 *   (nidmi, WiFi, calibrations PHY, etc.) — indispensable si NVS corrompue ou
 *   Preferences.begin() échoue.
 *
 * Dernier recours si l’upload échoue encore : effacement flash via esptool, ex. :
 *   esptool.py --chip esp32s3 erase_flash
 * (réinstalle ensuite le firmware + partition table avec ton flux habituel)
 */

#include <Preferences.h>
#include <nvs_flash.h>

void setup() {
    Serial.begin(115200);
    delay(1500);

    Serial.println();
    Serial.println("========================================");
    Serial.println("  NiDMI - Clear NVS (partition entiere)");
    Serial.println("========================================");
    Serial.println();

    /* 1) Tentative classique namespace NiDMI (optionnel si NVS saine) */
    Preferences preferences;
    if (preferences.begin("nidmi", false)) {
        bool cleared = preferences.clear();
        preferences.end();
        Serial.printf("[Clear NVS] Preferences namespace \"nidmi\" clear: %s\n",
                      cleared ? "OK" : "KO");
    } else {
        Serial.println("[Clear NVS] Preferences.begin(\"nidmi\") a echoue — NVS peut etre corrompue.");
        Serial.println("[Clear NVS] Passage a l'effacement brut de la partition NVS...");
    }

    /* 2) Effacement complet de la partition NVS (API ESP-IDF) */
    esp_err_t err = nvs_flash_deinit();
    Serial.printf("[Clear NVS] nvs_flash_deinit: %s (%d)\n", esp_err_to_name(err), (int)err);

    err = nvs_flash_erase();
    if (err == ESP_OK) {
        Serial.println("[Clear NVS] OK — partition NVS entiere effacee (WiFi, nidmi, tout).");
    } else {
        Serial.printf("[Clear NVS] ERREUR nvs_flash_erase: %s (%d)\n", esp_err_to_name(err), (int)err);
        Serial.println("[Clear NVS] Essayez esptool: esptool.py --chip esp32s3 erase_flash");
    }

    Serial.println();
    Serial.println("Redemarrage dans 2 secondes...");
    delay(2000);
    ESP.restart();
}

void loop() {}
