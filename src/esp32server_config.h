#pragma once

// Permettre au sketch d'override via un fichier local placé dans le sketch:
// Créez un fichier "esp32server_user_config.h" à côté du .ino avec vos #define
#if __has_include("esp32server_user_config.h")
#include "esp32server_user_config.h"
#endif

// Pas de valeurs par défaut ici: l'absence de #define maintient les fonctionnalités actives.
