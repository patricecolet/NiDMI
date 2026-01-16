#pragma once

#include <Arduino.h>

/**
 * @file MuxConstants.h
 * @brief Constantes pour les multiplexeurs analogiques
 * 
 * Centralise toutes les constantes liées aux multiplexeurs pour éviter
 * les dépendances inutiles et faciliter la maintenance.
 */

// Constantes pour les GPIO virtuels des multiplexeurs
// GPIO 200-215 = MUX0 canaux 0-15
// GPIO 216-231 = MUX1 canaux 0-15
// Limité à 2 multiplexeurs pour éviter le manque de pins digitales
static constexpr uint8_t MUX_GPIO_BASE = 200;
static constexpr uint8_t MUX_CHANNELS = 16;
static constexpr uint8_t MAX_MUXES = 2;
