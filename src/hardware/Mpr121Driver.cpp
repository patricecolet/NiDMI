#include "Mpr121Driver.h"

Mpr121Driver::Mpr121Driver(uint8_t address)
    : address_(address) {
}

bool Mpr121Driver::begin(uint8_t touch_thresh, uint8_t release_thresh) {
    Serial.printf("[Mpr121Driver] Init adresse 0x%02X...\n", address_);
    if (!I2CManager::isInitialized()) {
        I2CManager::begin();
        Serial.println("[Mpr121Driver] Bus I2C démarré");
    }

    // Plusieurs tentatives : le MPR121 peut être lent après mise sous tension
    for (int retry = 0; retry < 3; retry++) {
        if (retry > 0) {
            delay(50);
            Serial.printf("[Mpr121Driver] Retry %d/3...\n", retry + 1);
        }
        if (isConnected()) break;
        if (retry == 2) {
            Serial.printf("[Mpr121Driver] ERREUR: MPR121 non détecté à 0x%02X. Essayer 0x5B (2e module)? Câble 3.3V OK?\n", address_);
            return false;
        }
    }
    Serial.printf("[Mpr121Driver] Périphérique trouvé à 0x%02X\n", address_);

    // Soft reset : la puce peut NACK pendant le reset, on ignore l'erreur et on attend
    writeRegister(REG_SOFT_RESET, 0x63);
    delay(100);  // La puce a besoin de ~100 ms pour redémarrer après reset

    // Re-détection après reset (la puce peut avoir NACK juste avant le reset)
    for (int r = 0; r < 5; r++) {
        if (isConnected()) break;
        delay(30);
    }
    if (!isConnected()) {
        Serial.printf("[Mpr121Driver] ERREUR: plus de réponse après reset à 0x%02X\n", address_);
        return false;
    }

    // Stopper les électrodes avant la configuration
    if (!writeRegister(REG_ECR, 0x00)) {
        Serial.println("[Mpr121Driver] ERREUR: écriture ECR stop");
        return false;
    }
    delay(1);

    // === Filtrage baseline (AN3944 / Adafruit defaults) ===
    // Rising (release) : suivi rapide du baseline quand on relâche
    writeRegister(REG_MHD_R, 0x01);  // Max half delta rising
    writeRegister(REG_NHD_R, 0x01);  // Noise half delta rising
    writeRegister(REG_NCL_R, 0x0E);  // Noise count limit rising
    writeRegister(REG_FDL_R, 0x00);  // Filter delay count limit rising

    // Falling (touch) : suivi plus lent pour détecter les vrais touchs
    writeRegister(REG_MHD_F, 0x01);
    writeRegister(REG_NHD_F, 0x05);
    writeRegister(REG_NCL_F, 0x01);
    writeRegister(REG_FDL_F, 0x00);

    // Touched : maintien du baseline pendant le touch
    writeRegister(REG_NHD_T, 0x00);
    writeRegister(REG_NCL_T, 0x00);
    writeRegister(REG_FDL_T, 0x00);

    // === Seuils touch/release pour les 12 électrodes ===
    if (touch_thresh < 1) touch_thresh = 1;
    if (touch_thresh > 50) touch_thresh = 50;
    if (release_thresh >= touch_thresh) release_thresh = touch_thresh / 2;
    if (release_thresh < 1) release_thresh = 1;
    Serial.printf("[Mpr121Driver] Seuils: touch=%d release=%d\n", touch_thresh, release_thresh);
    for (uint8_t e = 0; e < 12; e++) {
        writeRegister(REG_E0_TTH + e * 2, touch_thresh);
        writeRegister(REG_E0_TTH + e * 2 + 1, release_thresh);
    }

    // === Debounce : 2 touch, 2 release (réactif pour gros pads) ===
    writeRegister(REG_DEBOUNCE, 0x22);

    // === Configuration filtrage global (optimisé gros pads / fils) ===
    // CONFIG1 (0x5C): FFI=00 (6 samples), CDC=63µA (0x3F) — courant max pour gros pads
    writeRegister(REG_CONFIG1, 0x3F);
    // CONFIG2 (0x5D): CDT=111(32µs), SFI=00(4 samples), ESI=001(2ms)
    // 0xE1 = 1110_0001 — temps de charge max pour capacité parasite élevée
    writeRegister(REG_CONFIG2, 0xE1);

    // === Auto-configuration (3.3V) ===
    // Calculs pour VDD = 3.3V (AN3944)
    // USL = 256 * (VDD - 0.7) / VDD = 256 * 2.6/3.3 ≈ 202
    // LSL = USL * 0.65 ≈ 131
    // TL  = USL * 0.9  ≈ 182
    writeRegister(REG_USL, 202);
    writeRegister(REG_LSL, 131);
    writeRegister(REG_TL,  182);
    // AUTO_CFG0: activer auto-config + auto-recalibration, retry=2
    writeRegister(REG_AUTO_CFG0, 0x0B);
    // AUTO_CFG1: OOR si hors limites (baseline out-of-range → recalibration)
    writeRegister(REG_AUTO_CFG1, 0x00);

    // === Activer 12 électrodes avec baseline tracking ===
    // ECR = 0x8F : CL=10 (baseline tracking), ELEPROX_EN=00, ELE_EN=1111 (12 electrodes)
    if (!writeRegister(REG_ECR, 0x8F)) {
        Serial.println("[Mpr121Driver] ERREUR: écriture ECR run");
        return false;
    }

    Serial.printf("[Mpr121Driver] MPR121 OK à 0x%02X (12 électrodes, auto-config, debounce)\n", address_);
    return true;
}

bool Mpr121Driver::isConnected() {
    return I2CManager::deviceExists(address_);
}

bool Mpr121Driver::readTouchStatus(uint16_t& out_mask) {
    uint8_t lo = readRegister(REG_TOUCH_STATUS_L);
    uint8_t hi = readRegister(REG_TOUCH_STATUS_H);

    // Bit 7 du registre 0x01 = Over Current Flag (court-circuit ou câblage défectueux)
    if (hi & 0x80) {
        static unsigned long lastOvrLog = 0;
        if (millis() - lastOvrLog > 5000) {
            Serial.printf("[Mpr121Driver] ATTENTION: Over Current Flag à 0x%02X (vérifier câblage électrodes)\n", address_);
            lastOvrLog = millis();
        }
        // Soft reset et re-init nécessaire en cas d'OVR
        writeRegister(REG_ECR, 0x00);
        delay(1);
        writeRegister(REG_ECR, 0x8F);
        out_mask = 0;
        return false;
    }

    out_mask = (static_cast<uint16_t>(hi & 0x0F) << 8) | lo;
    return true;
}

bool Mpr121Driver::readElectrodeData(uint8_t electrode, uint16_t& filtered, uint16_t& baseline) {
    if (electrode > 11) return false;
    // Filtered data: 2 bytes per electrode at 0x04 + electrode*2
    uint8_t filt_lo = readRegister(0x04 + electrode * 2);
    uint8_t filt_hi = readRegister(0x05 + electrode * 2);
    filtered = ((uint16_t)(filt_hi & 0x03) << 8) | filt_lo;
    // Baseline: 1 byte per electrode at 0x1E + electrode (upper 8 bits, <<2 = 10-bit)
    uint8_t bl = readRegister(0x1E + electrode);
    baseline = (uint16_t)bl << 2;
    return true;
}

void Mpr121Driver::logDiagnostic() {
    Serial.printf("[MPR121 diag] E: ");
    for (uint8_t e = 0; e < 12; e++) {
        uint16_t filt, base;
        if (readElectrodeData(e, filt, base)) {
            int16_t delta = (int16_t)base - (int16_t)filt;
            Serial.printf("%d:%d/%d(%+d) ", e, filt, base, delta);
        }
    }
    Serial.println();
}

uint8_t Mpr121Driver::readRegister(uint8_t reg) {
    return I2CManager::readRegister(address_, reg);
}

bool Mpr121Driver::writeRegister(uint8_t reg, uint8_t value) {
    return I2CManager::writeRegister(address_, reg, value);
}
