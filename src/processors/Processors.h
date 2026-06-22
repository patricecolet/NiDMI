#pragma once

/**
 * @file Processors.h
 * @brief Centralise tous les includes de processeurs pour garantir leur enregistrement automatique
 * 
 * Ce fichier centralise tous les includes de processeurs pour garantir
 * leur enregistrement automatique via les constructeurs statiques.
 * 
 * Inclure ce fichier dans ComponentManager.cpp au lieu d'inclure chaque processeur individuellement.
 * 
 * Pour ajouter un nouveau processeur :
 * 1. Créer MyComponentProcessor.h/cpp dans src/processors/
 * 2. Ajouter #include "MyComponentProcessor.h" ci-dessous
 * 3. L'enregistrement se fera automatiquement via le constructeur statique dans le .cpp
 */

#include "PotentiometerProcessor.h"
#include "ButtonProcessor.h"
#include "LedProcessor.h"
#include "VelostatProcessor.h"
#include "UltrasonicProcessor.h"
#include "TouchProcessor.h"
#include "ImuProcessor.h"
#include "Mpr121Processor.h"
#include "NoiseSamplerProcessor.h"

// Pour ajouter un nouveau processeur :
// #include "MyComponentProcessor.h"
