#pragma once

#include <stdint.h>

/*
 * Ports OSC par défaut — une seule définition pour tous les lecteurs de la NVS.
 *
 * La page, l'API et l'émetteur relisaient chacun « osc_port » avec leur propre
 * valeur de repli (8000 d'un côté, 8001 de l'autre) : quand la clé manquait ou
 * n'avait pas le bon type, la page affichait 8000 pendant que l'OSC partait sur
 * 8001, sans que rien ne le montre.
 */
namespace osc_defaults {

/* Port distant : là où partent les messages (clé NVS « osc_port »). */
constexpr uint16_t kRemotePort = 8000;

/* Port local : là où la carte écoute l'OSC entrant (commandes de calibrage). */
constexpr uint16_t kLocalPort = 8001;

}  // namespace osc_defaults
