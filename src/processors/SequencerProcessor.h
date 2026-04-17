#pragma once

/**
 * @file SequencerProcessor.h
 * @brief Moteur séquenceur NiDMI - Parsing et lecture du fichier binaire .nidmid
 * 
 * Ce module fournit:
 * - parseNidmid() : Parse un buffer binaire en steps/notes
 * - reloadSequencerFromStorage() : Recharge depuis LittleFS (après upload ou au boot)
 * - Accès aux données: steps[], stepCount
 * 
 * Architecture:
 * - Données globales: step[MAX_STEPS], stepCount
 * - Au boot: reloadSequencerFromStorage() lit le fichier depuis LittleFS
 * - Après upload HTTP: reloadSequencerFromStorage() est appelé pour hot-reload immédiat
 * - Erreurs: logs série, sequencer reste inchangé si fichier absent/invalide
 */

#include <cstdint>
#include <cstddef>

// ============================================================================
// Structures de données du séquenceur
// ============================================================================

#define MAX_STEPS 32
#define MAX_NOTES 4

struct Note {
    uint8_t pitch;
    uint8_t velocity;
};

struct Step {
    uint8_t noteCount;
    Note notes[MAX_NOTES];
    uint8_t measure;
};

// ============================================================================
// Variables globales externes (déclarées dans SequencerProcessor.cpp)
// ============================================================================

extern Step steps[MAX_STEPS];
extern uint8_t stepCount;

// ============================================================================
// Fonctions publiques
// ============================================================================

/**
 * @brief Parser un buffer binaire .nidmid en steps/notes
 * 
 * Format binaire:
 * - 0xFF: nouveau mesure (incremente counter)
 * - 0x00: pas de note
 * - 0x01-0x04: nombre de notes suivantes
 * - Chaque note: [pitch (1 byte), velocity (1 byte)]
 * 
 * @param data Buffer d'entrée
 * @param len Taille du buffer
 * 
 * @note Efface le contenu précédent de steps[] et met à jour stepCount
 */
void parseNidmid(uint8_t* data, size_t len);

/**
 * @brief Recharger la séquence depuis le stockage LittleFS
 * 
 * Comportement:
 * - Lit le fichier depuis LittleFS (/seq/nidmid.bin)
 * - Valide l'intégrité (CRC32 si en-tête disponible)
 * - Parse le contenu avec parseNidmid()
 * - Logs série en cas de succès ou erreur
 * - Si erreur: sequencer conserve l'état précédent (pas de modification)
 * 
 * Utilisation:
 * - Au boot (nidmi_begin): Charge la séquence sauvegardée
 * - Après upload HTTP: Hot-reload immédiat pour utilisation immédiate
 * - Au runtime: Réinitialisation manuelle possible
 * 
 * @return true si chargement réussi, false sinon (fichier absent, invalide, etc.)
 * 
 * @note Cette fonction est sûre à appeler à tout moment
 * @note Les erreurs sont loggées en série mais ne bloquent pas le système
 */
bool reloadSequencerFromStorage();

/**
 * @brief Obtenir le nombre d'étapes actuellement chargées
 * 
 * @return Nombre d'étapes (0 si néant chargé ou fichier absent)
 */
inline uint8_t getStepCount() {
    return stepCount;
}

/**
 * @brief Obtenir le contenu de l'étape i (pour debug/monitoring)
 * 
 * @param index Index de l'étape (0 à MAX_STEPS-1)
 * @return Pointeur sur Step si valide, nullptr sinon
 */
inline Step* getStep(uint8_t index) {
    if (index < MAX_STEPS && index < stepCount) return &steps[index];
    return nullptr;
}
