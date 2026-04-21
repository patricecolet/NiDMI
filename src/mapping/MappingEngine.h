#pragma once
#include <Arduino.h>

// Forward declaration
class MidiSender;

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
    static bool has(const char* name);
    static void debug();  // Print registry contents for debugging
};

// 2. Le Moteur : Découpe et exécute le script segment par segment
class MappingEngine {
public:
    // Utilisation de const char* pour être plus léger que String
    static void execute(const char* script, float inputVal, MidiSender* midi_sender = nullptr);
};