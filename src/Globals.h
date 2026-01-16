#pragma once

/**
 * @file Globals.h
 * @brief Déclarations centralisées des variables globales
 * 
 * Ce fichier centralise toutes les déclarations externes de variables globales
 * pour éviter les déclarations dupliquées dans plusieurs fichiers.
 */

// Forward declarations
class ComponentManager;
class MidiRouter;
class ConfigCache;
class DebugManager;
class ServerCore;

// Variables globales principales
extern ComponentManager g_componentManager;
extern MidiRouter g_midiRouter;
extern ConfigCache g_configCache;
extern DebugManager* g_debug;
extern ServerCore serverCore;
