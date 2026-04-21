#pragma once

#include "ComponentDefinition.h"

/**
 * @file MidiMessageFactory.h
 * @brief Factory functions pour créer des messages MIDI communs
 * 
 * Ces fonctions permettent de créer des MidiMessageDef standardisés
 * sans répéter le code pour chaque composant.
 */

namespace Components {

/**
 * @brief Crée un message Control Change (CC)
 * @param includeRange Si true, ajoute le paramètre midiCcRange (pour potentiomètres)
 * @param dependsOnRole JSON array des rôles qui affichent le range (ex: "[\"potentiometer\"]")
 */
inline MidiMessageDef createCcMessage(bool includeRange = false, const char* dependsOnRole = nullptr) {
    MidiMessageDef msg;
    msg.id = "cc";
    msg.displayName = "Control Change";
    msg.statusTemplate = "CC#{cc}";
    
    if (includeRange) {
        msg.paramCount = 3;
        msg.paramsCapacity = 3;
        msg.params = new MidiParamDef[3];
        
        msg.params[0] = MidiParamDef{"midiCc", "{{t.pins.cc}}:", FieldType::NUMBER, 0, 127, "7", "7", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        msg.params[1] = MidiParamDef{"midiChannel", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        msg.params[2] = MidiParamDef{"midiCcRange", "{{t.pins.midiRange}}:", FieldType::RANGE, 0, 127, nullptr, nullptr, "0", "127", "→", nullptr, nullptr, 90, dependsOnRole};
    } else {
        msg.paramCount = 2;
        msg.paramsCapacity = 2;
        msg.params = new MidiParamDef[2];
        
        msg.params[0] = MidiParamDef{"midiCc", "{{t.pins.cc}}:", FieldType::NUMBER, 0, 127, "7", "7", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        msg.params[1] = MidiParamDef{"midiChannel", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
    }
    
    return msg;
}

/**
 * @brief Crée un message Note On/Off
 * @param includeVelocity Si true, ajoute le paramètre midiVelocity (pour boutons)
 * @param dependsOnRole JSON array des rôles qui affichent la vélocité (ex: "[\"button\"]")
 */
inline MidiMessageDef createNoteMessage(bool includeVelocity = false, const char* dependsOnRole = nullptr) {
    MidiMessageDef msg;
    msg.id = "note";
    msg.displayName = "Note";
    msg.statusTemplate = "Note {note}";
    
    if (includeVelocity) {
        msg.paramCount = 3;
        msg.paramsCapacity = 3;
        msg.params = new MidiParamDef[3];
        
        msg.params[0] = MidiParamDef{"midiNote", "{{t.pins.note}}:", FieldType::NUMBER, 0, 127, "60", "60", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        msg.params[1] = MidiParamDef{"midiChannel", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        msg.params[2] = MidiParamDef{"midiVelocity", "{{t.pins.velocity}}:", FieldType::NUMBER, 1, 127, "100", "100", nullptr, nullptr, nullptr, nullptr, nullptr, 90, dependsOnRole};
    } else {
        msg.paramCount = 2;
        msg.paramsCapacity = 2;
        msg.params = new MidiParamDef[2];
        
        msg.params[0] = MidiParamDef{"midiNote", "{{t.pins.note}}:", FieldType::NUMBER, 0, 127, "60", "60", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
        msg.params[1] = MidiParamDef{"midiChannel", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
    }
    
    return msg;
}

/**
 * @brief Crée un message Program Change (PC)
 */
inline MidiMessageDef createPcMessage() {
    MidiMessageDef msg;
    msg.id = "pc";
    msg.displayName = "Program Change";
    msg.statusTemplate = "PC#{pc}";
    msg.paramCount = 2;
    msg.paramsCapacity = 2;
    msg.params = new MidiParamDef[2];
    
    msg.params[0] = MidiParamDef{"midiPc", "{{t.pins.program}}:", FieldType::NUMBER, 1, 128, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
    msg.params[1] = MidiParamDef{"midiChannel", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
    
    return msg;
}

/**
 * @brief Crée un message Pitch Bend
 */
inline MidiMessageDef createPitchBendMessage() {
    MidiMessageDef msg;
    msg.id = "pitchbend";
    msg.displayName = "Pitch Bend";
    msg.statusTemplate = "Pitch Bend";
    msg.paramCount = 1;
    msg.paramsCapacity = 1;
    msg.params = new MidiParamDef[1];
    
    msg.params[0] = MidiParamDef{"midiChannel", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
    
    return msg;
}

/**
 * @brief Crée un message Aftertouch (Channel)
 */
inline MidiMessageDef createAftertouchMessage() {
    MidiMessageDef msg;
    msg.id = "aftertouch";
    msg.displayName = "Aftertouch (Channel)";
    msg.statusTemplate = "Aftertouch";
    msg.paramCount = 1;
    msg.paramsCapacity = 1;
    msg.params = new MidiParamDef[1];
    
    msg.params[0] = MidiParamDef{"midiChannel", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
    
    return msg;
}

/**
 * @brief Crée un message Control Change avec valeurs ON/OFF (pour boutons)
 * @param dependsOnRole JSON array des rôles qui affichent le range ON/OFF (ex: "[\"button\"]")
 */
inline MidiMessageDef createCcOnOffMessage(const char* dependsOnRole = nullptr) {
    MidiMessageDef msg;
    msg.id = "cc";
    msg.displayName = "Control Change";
    msg.statusTemplate = "CC#{cc}";
    msg.paramCount = 3;
    msg.paramsCapacity = 3;
    msg.params = new MidiParamDef[3];
    
    msg.params[0] = MidiParamDef{"midiCc", "{{t.pins.cc}}:", FieldType::NUMBER, 0, 127, "7", "7", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
    msg.params[1] = MidiParamDef{"midiChannel", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
    msg.params[2] = MidiParamDef{"midiCcOnOff", "{{t.pins.values}}:", FieldType::RANGE, 0, 127, nullptr, nullptr, "127", "0", "→", nullptr, nullptr, 90, dependsOnRole};
    
    return msg;
}

/**
 * @brief Crée un message Note Sweep (balayage de notes)
 */
inline MidiMessageDef createNoteSweepMessage() {
    MidiMessageDef msg;
    msg.id = "notesweep";
    msg.displayName = "Note (balayage)";
    msg.statusTemplate = "Note {note} scan";
    msg.paramCount = 4;
    msg.paramsCapacity = 4;
    msg.params = new MidiParamDef[4];
    
    msg.params[0] = MidiParamDef{"midiNoteSweep", "{{t.pins.sweep}}:", FieldType::RANGE, 0, 127, nullptr, nullptr, "48", "72", "→", nullptr, nullptr, 90, nullptr};
    msg.params[1] = MidiParamDef{"midiNoteVelocityFix", "{{t.pins.fixedVelocity}}:", FieldType::NUMBER, 1, 127, "100", "100", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
    msg.params[2] = MidiParamDef{"midiNoteSweepAutoOffDelay", "{{t.pins.autoOff}}", FieldType::NUMBER, 0, 65535, "1000", "1000", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
    msg.params[3] = MidiParamDef{"midiChannel", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
    
    return msg;
}

/**
 * @brief Crée un message Clock (sans paramètres, juste un hint)
 */
inline MidiMessageDef createClockMessage() {
    MidiMessageDef msg;
    msg.id = "clock";
    msg.displayName = "Clock";
    msg.statusTemplate = "Clock";
    msg.paramCount = 1;
    msg.paramsCapacity = 1;
    msg.params = new MidiParamDef[1];
    
    msg.params[0] = MidiParamDef{"rtpClockHint", nullptr, FieldType::INFO, 0, 0, nullptr, nullptr, nullptr, nullptr, nullptr, "{{t.pins.clockHint}}", "color:#6b7280;", 0, nullptr};
    
    return msg;
}

/**
 * @brief Crée un message Note avec Key Pressure (pour Velostat)
 */
inline MidiMessageDef createNoteWithKeyPressureMessage() {
    MidiMessageDef msg;
    msg.id = "note";
    msg.displayName = "Note + Key Pressure";
    msg.statusTemplate = "Note {note} + Key Press";
    msg.paramCount = 2;
    msg.paramsCapacity = 2;
    msg.params = new MidiParamDef[2];
    
    msg.params[0] = MidiParamDef{"midiNote", "{{t.pins.note}}:", FieldType::NUMBER, 0, 127, "60", "60", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
    msg.params[1] = MidiParamDef{"midiChannel", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr};
    
    return msg;
}

// Chaînes statiques pour les displayName avec axes
namespace {
    constexpr const char* CC_DISPLAY_NAME_X = "Control Change (Axe X)";
    constexpr const char* CC_DISPLAY_NAME_Y = "Control Change (Axe Y)";
    constexpr const char* CC_DISPLAY_NAME_Z = "Control Change (Axe Z)";
    constexpr const char* PB_DISPLAY_NAME_X = "Pitch Bend (Axe X)";
    constexpr const char* PB_DISPLAY_NAME_Y = "Pitch Bend (Axe Y)";
    constexpr const char* PB_DISPLAY_NAME_Z = "Pitch Bend (Axe Z)";
    constexpr const char* AT_DISPLAY_NAME_X = "Aftertouch (Axe X)";
    constexpr const char* AT_DISPLAY_NAME_Y = "Aftertouch (Axe Y)";
    constexpr const char* AT_DISPLAY_NAME_Z = "Aftertouch (Axe Z)";
    constexpr const char* NS_DISPLAY_NAME_X = "Note (balayage) (Axe X)";
    constexpr const char* NS_DISPLAY_NAME_Y = "Note (balayage) (Axe Y)";
    constexpr const char* NS_DISPLAY_NAME_Z = "Note (balayage) (Axe Z)";
}

/**
 * @brief Helper pour comparer un axe avec "x"
 */
inline bool isAxisX(const char* axis) {
    return axis && axis[0] == 'x' && axis[1] == '\0';
}

/**
 * @brief Helper pour comparer un axe avec "z"
 */
inline bool isAxisZ(const char* axis) {
    return axis && axis[0] == 'z' && axis[1] == '\0';
}

/**
 * @brief Crée un message Control Change pour un axe (joystick ou IMU)
 * @param axis Axe ("x", "y" ou "z")
 * @param includeRange Si true, ajoute le paramètre midiCcRange
 * @param dependsOnRole JSON array des rôles qui affichent le range (ex: "[\"joystick\"]")
 */
inline MidiMessageDef createCcMessageForAxis(const char* axis, bool includeRange = false, const char* dependsOnRole = nullptr) {
    MidiMessageDef msg = createCcMessage(includeRange, dependsOnRole);
    msg.axis = axis;
    if (isAxisX(axis)) {
        msg.displayName = CC_DISPLAY_NAME_X;
    } else if (isAxisZ(axis)) {
        msg.displayName = CC_DISPLAY_NAME_Z;
    } else {
        msg.displayName = CC_DISPLAY_NAME_Y;
    }
    return msg;
}

/**
 * @brief Crée un message Pitch Bend pour un axe (joystick ou IMU)
 * @param axis Axe ("x", "y" ou "z")
 */
inline MidiMessageDef createPitchBendMessageForAxis(const char* axis) {
    MidiMessageDef msg = createPitchBendMessage();
    msg.axis = axis;
    if (isAxisX(axis)) {
        msg.displayName = PB_DISPLAY_NAME_X;
    } else if (isAxisZ(axis)) {
        msg.displayName = PB_DISPLAY_NAME_Z;
    } else {
        msg.displayName = PB_DISPLAY_NAME_Y;
    }
    return msg;
}

/**
 * @brief Crée un message Aftertouch pour un axe (joystick ou IMU)
 * @param axis Axe ("x", "y" ou "z")
 */
inline MidiMessageDef createAftertouchMessageForAxis(const char* axis) {
    MidiMessageDef msg = createAftertouchMessage();
    msg.axis = axis;
    if (isAxisX(axis)) {
        msg.displayName = AT_DISPLAY_NAME_X;
    } else if (isAxisZ(axis)) {
        msg.displayName = AT_DISPLAY_NAME_Z;
    } else {
        msg.displayName = AT_DISPLAY_NAME_Y;
    }
    return msg;
}

/**
 * @brief Crée un message Note Sweep pour un axe (joystick ou IMU)
 * @param axis Axe ("x", "y" ou "z")
 */
inline MidiMessageDef createNoteSweepMessageForAxis(const char* axis) {
    MidiMessageDef msg = createNoteSweepMessage();
    msg.axis = axis;
    if (isAxisX(axis)) {
        msg.displayName = NS_DISPLAY_NAME_X;
    } else if (isAxisZ(axis)) {
        msg.displayName = NS_DISPLAY_NAME_Z;
    } else {
        msg.displayName = NS_DISPLAY_NAME_Y;
    }
    return msg;
}

} // namespace Components
