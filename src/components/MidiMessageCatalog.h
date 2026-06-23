#pragma once

#include "ComponentDefinition.h"

/**
 * @file MidiMessageCatalog.h
 * @brief Catalogue de messages MIDI partagés, stockés en FLASH (.rodata).
 *
 * Contrairement à MidiMessageFactory.h (qui alloue les params en heap via new[]
 * à chaque appel, dupliqués pour chaque composant), ce catalogue expose des
 * tableaux `inline constexpr` UNIQUES en flash, partagés par tous les composants.
 *
 * Gain : ~0 octet de heap pour les messages standard (vs ~24-40 o par message
 * dupliqué × dizaines de composants). Voir le refactor "Defs en flash".
 *
 * Les définitions qui utilisent ce catalogue mettent `ownsArrays = false` :
 * cleanup() ne libère jamais ces tableaux (ils sont en flash).
 */

namespace Components {

/* ---- Paramètres canoniques partagés (flash, dédupliqués) ----
 * Ordre du ctor MidiParamDef :
 * (id, label, type, min, max, placeholder, defaultValue,
 *  defaultMin, defaultMax, separator, hint, hintClass, width, dependsOnRole)
 */

inline constexpr MidiParamDef PARAM_CC_CHANNEL[] = {
    {"midiCc",      "{{t.pins.cc}}:",      FieldType::NUMBER, 0, 127, "7", "7", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr},
    {"midiChannel", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16,  "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr},
};

inline constexpr MidiParamDef PARAM_NOTE_CHANNEL[] = {
    {"midiNote",    "{{t.pins.note}}:",    FieldType::NUMBER, 0, 127, "60", "60", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr},
    {"midiChannel", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16,  "1",  "1",  nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr},
};

inline constexpr MidiParamDef PARAM_PC_CHANNEL[] = {
    {"midiPc",      "{{t.pins.program}}:", FieldType::NUMBER, 1, 128, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr},
    {"midiChannel", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16,  "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr},
};

inline constexpr MidiParamDef PARAM_CHANNEL_ONLY[] = {
    {"midiChannel", "{{t.pins.channel}}:", FieldType::NUMBER, 1, 16, "1", "1", nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr},
};

inline constexpr MidiParamDef PARAM_NOTESWEEP[] = {
    {"midiNoteSweep",             "{{t.pins.sweep}}:",         FieldType::RANGE,  0, 127,   nullptr, nullptr, "48",   "72",   "→",     nullptr, nullptr, 90, nullptr},
    {"midiNoteVelocityFix",       "{{t.pins.fixedVelocity}}:", FieldType::NUMBER, 1, 127,   "100",   "100",   nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr},
    {"midiNoteSweepAutoOffDelay", "{{t.pins.autoOff}}",        FieldType::NUMBER, 0, 65535, "1000",  "1000",  nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr},
    {"midiChannel",               "{{t.pins.channel}}:",       FieldType::NUMBER, 1, 16,    "1",     "1",     nullptr, nullptr, nullptr, nullptr, nullptr, 90, nullptr},
};

inline constexpr MidiParamDef PARAM_CLOCK[] = {
    {"rtpClockHint", nullptr, FieldType::INFO, 0, 0, nullptr, nullptr, nullptr, nullptr, nullptr, "{{t.pins.clockHint}}", "color:#6b7280;", 0, nullptr},
};

/* ---- Messages standard partagés (flash) ----
 * Ordre du ctor MidiMessageDef :
 * (id, displayName, statusTemplate, axis, paramCount, params, paramsCapacity)
 */

constexpr MidiMessageDef msgCc()         { return {"cc",         "Control Change",        "CC#{cc}",       nullptr, 2, PARAM_CC_CHANNEL,   2}; }
constexpr MidiMessageDef msgNote()       { return {"note",       "Note",                  "Note {note}",   nullptr, 2, PARAM_NOTE_CHANNEL, 2}; }
constexpr MidiMessageDef msgPc()         { return {"pc",         "Program Change",        "PC#{pc}",       nullptr, 2, PARAM_PC_CHANNEL,   2}; }
constexpr MidiMessageDef msgPitchBend()  { return {"pitchbend",  "Pitch Bend",            "Pitch Bend",    nullptr, 1, PARAM_CHANNEL_ONLY, 1}; }
constexpr MidiMessageDef msgAftertouch() { return {"aftertouch", "Aftertouch (Channel)",  "Aftertouch",    nullptr, 1, PARAM_CHANNEL_ONLY, 1}; }
constexpr MidiMessageDef msgNoteSweep()  { return {"notesweep",  "Note (balayage)",       "Note {note} scan", nullptr, 4, PARAM_NOTESWEEP, 4}; }
constexpr MidiMessageDef msgClock()      { return {"clock",      "Clock",                 "Clock",         nullptr, 1, PARAM_CLOCK,        1}; }
constexpr MidiMessageDef msgNoteKeyPressure() { return {"note",  "Note + Key Pressure",   "Note {note} + Key Press", nullptr, 2, PARAM_NOTE_CHANNEL, 2}; }

/**
 * @brief Assemble une ComponentDefinition pointant vers des tableaux flash.
 * ownsArrays = false → cleanup() ne libère rien (tout est en .rodata).
 */
inline ComponentDefinition makeFlashDef(
    const char* id, const char* displayName, const char* cardId,
    ComponentFamily family, const char* familyName,
    ComponentType type, PinType pinType,
    bool supportsMidi, bool supportsOsc, bool implemented,
    const FormFieldDef* formFields, uint8_t formFieldCount,
    const MidiMessageDef* midiMessages, uint8_t midiMessageCount,
    const char* statusValueMappings = nullptr,
    const AdditionalPinDef* additionalPins = nullptr, uint8_t additionalPinCount = 0)
{
    ComponentDefinition d;
    d.id = id;
    d.displayName = displayName;
    d.cardId = cardId;
    d.family = family;
    d.familyName = familyName;
    d.type = type;
    d.pinType = pinType;
    d.supportsMidi = supportsMidi;
    d.supportsOsc = supportsOsc;
    d.implemented = implemented;
    d.statusValueMappings = statusValueMappings;
    d.formFields = formFields;
    d.formFieldCount = formFieldCount;
    d.formFieldsCapacity = formFieldCount;
    d.midiMessages = midiMessages;
    d.midiMessageCount = midiMessageCount;
    d.midiMessagesCapacity = midiMessageCount;
    d.additionalPins = additionalPins;
    d.additionalPinCount = additionalPinCount;
    d.additionalPinsCapacity = additionalPinCount;
    d.ownsArrays = false;  // tableaux en flash → ne pas delete[]
    return d;
}

} // namespace Components
