#pragma once

/**
 * @file Definitions.h
 * @brief Centralise tous les includes de définitions pour garantir leur enregistrement automatique
 * 
 * Ce fichier centralise tous les includes de définitions pour garantir
 * leur enregistrement automatique via les constructeurs statiques dans les fichiers .cpp.
 * 
 * Inclure ce fichier dans ComponentRegistry.cpp au lieu d'inclure chaque définition individuellement.
 * 
 * Pour ajouter une nouvelle définition :
 * 1. Créer MyComponentDef.h/cpp dans src/components/[famille]/
 * 2. Ajouter #include "[famille]/MyComponentDef.h" ci-dessous
 * 3. L'enregistrement se fera automatiquement via le constructeur statique dans le .cpp
 */

// === FAMILLE BASIC ===
#include "basic/PotentiometerDef.h"
#include "basic/ButtonDef.h"
#include "basic/LedDef.h"
#include "basic/VelostatDef.h"
#include "basic/UltrasonicDef.h"
#include "basic/TouchDef.h"

// === FAMILLE MULTIPLEXER ===
#include "multiplexer/MuxDef.h"

// Pour ajouter une nouvelle définition :
// #include "[famille]/MyComponentDef.h"
