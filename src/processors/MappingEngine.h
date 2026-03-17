#pragma once
#include <Arduino.h>

// 1. Le Registre : Stocke les valeurs de chaque composant par nom
class FluxRegistry {
public:
    struct Entry { 
        char name[16]; 
        float value; 
    };
    
    static Entry entries[32]; 
    static int count;

    static void update(const char* name, float val);
    static float get(const char* name);
};

// 2. Le Moteur : Découpe et exécute le script segment par segment
class MappingEngine {
public:
    // Utilisation de const char* pour être plus léger que String
    static void execute(const char* script, float inputVal);
};