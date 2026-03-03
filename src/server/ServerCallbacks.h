#pragma once

/**
 * @file ServerCallbacks.h
 * @brief Callbacks C pour les fonctions externes
 * 
 * Centralise les déclarations de callbacks C utilisées par l'API
 * pour éviter les dépendances inutiles vers NiDMIServer.h
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Demander le rechargement des configurations de pins
 * 
 * Cette fonction est appelée depuis l'API web pour demander
 * un rechargement des configurations de pins depuis la NVS.
 */
void nidmi_requestReloadPins();

/**
 * @brief Demander le rechargement de la configuration OSC
 * 
 * Cette fonction peut être appelée depuis l'API web pour demander
 * un rechargement de la configuration OSC.
 */
void nidmi_requestReloadOsc();

/**
 * @brief Demander un redémarrage différé (2s pour laisser la réponse HTTP partir)
 */
void nidmi_requestReboot();


#ifdef __cplusplus
}
#endif
