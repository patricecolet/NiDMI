#pragma once

#include <Arduino.h>

/**
 * @brief Parser JSON simple et optimisé pour Arduino
 * 
 * Parse des chaînes JSON simples sans bibliothèque externe.
 * Utilisé pour extraire des valeurs depuis les configurations NVS.
 */
class JSONParser {
public:
    /**
     * @brief Extraire un entier depuis une chaîne JSON
     * @param src Chaîne JSON source
     * @param key Clé à rechercher (ex: "rtpCc")
     * @param def Valeur par défaut si la clé n'est pas trouvée
     * @return Valeur entière trouvée ou valeur par défaut
     */
    static int extractInt(const String& src, const char* key, int def);
    
    /**
     * @brief Extraire un booléen depuis une chaîne JSON
     * @param src Chaîne JSON source
     * @param key Clé à rechercher (ex: "oscEnabled")
     * @param def Valeur par défaut si la clé n'est pas trouvée
     * @return Valeur booléenne trouvée ou valeur par défaut
     */
    static bool extractBool(const String& src, const char* key, bool def);
    
    /**
     * @brief Extraire une chaîne depuis une chaîne JSON
     * @param src Chaîne JSON source
     * @param key Clé à rechercher (ex: "role")
     * @param def Valeur par défaut si la clé n'est pas trouvée
     * @return Chaîne trouvée ou valeur par défaut
     */
    static String extractStr(const String& src, const char* key, const String& def);
};
