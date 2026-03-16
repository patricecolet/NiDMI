#pragma once

/**
 * @file Handlers.h
 * @brief Centralise tous les includes de handlers pour garantir leur enregistrement automatique
 *
 * Ce fichier centralise tous les includes de handlers pour garantir
 * leur enregistrement automatique via les constructeurs statiques dans les fichiers .cpp.
 *
 * Inclure ce fichier dans ComplexHandlerRegistry.cpp au lieu d'inclure chaque handler individuellement.
 *
 * Pour ajouter un nouveau handler :
 * 1. Créer MyHandler.h/cpp dans src/managers/complex/[famille]/
 * 2. Ajouter #include "[famille]/MyHandler.h" ci-dessous
 * 3. L'enregistrement se fera automatiquement via le constructeur statique dans le .cpp
 */

/**
 * @file Handlers.h
 * @brief Centralise tous les includes de handlers
 * 
 * Ce fichier centralise tous les includes de handlers pour garantir
 * qu'ils sont disponibles. L'enregistrement automatique se fait dans
 * les fichiers *Register.cpp qui sont inclus dans ComplexHandlerRegistry.cpp
 */

// === FAMILLE MULTIPLEXER ===
#include "multiplexer/MuxHandler.h"

// Pour ajouter un nouveau handler :
// 1. Ajouter #include "[famille]/MyHandler.h" ci-dessus
// 2. Créer MyHandlerRegister.cpp avec l'enregistrement automatique
// 3. Ajouter #include "[famille]/MyHandlerRegister.cpp" dans ComplexHandlerRegistry.cpp
