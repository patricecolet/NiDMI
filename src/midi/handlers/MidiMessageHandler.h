#pragma once

#include <Arduino.h>
#include "../../components/ComponentTypes.h"
#include "../../components/basic/PotentiometerDef.h"  // Pour PotentiometerConfig
#include "../MidiSender.h"
#include "../MidiMessageType.h"

/**
 * @file MidiMessageHandler.h
 * @brief Interface et handlers spécialisés pour l'envoi de messages MIDI
 * 
 * Architecture basée sur le Strategy Pattern pour éviter les redondances
 * et permettre des traitements spécifiques par type de message.
 */

/**
 * @brief Interface de base pour les handlers de messages MIDI
 */
class MidiMessageHandler {
public:
    virtual ~MidiMessageHandler() {}
    
    /**
     * @brief Envoie un message MIDI selon le handler
     * 
     * @param sender Interface d'envoi MIDI
     * @param config Configuration du composant
     * @param value Valeur MIDI (0-127 pour la plupart)
     * @param raw_value Valeur brute (0-4095) pour handlers nécessitant plus de résolution (ex: pitchbend)
     */
    virtual void send(MidiSender* sender, const ComponentConfig& config, uint8_t value, uint16_t raw_value = 0) = 0;
    
    /**
     * @brief Indique si le handler supporte des valeurs continues (true) ou discrètes (false)
     */
    virtual bool supportsValueRange() const { return true; }
    
    /**
     * @brief Indique si le handler nécessite des paramètres supplémentaires
     * (ex: NOTE_SWEEP nécessite noteMin/Max/VelFix)
     */
    virtual bool requiresSpecialParams() const { return false; }
};

/**
 * @brief Handler pour Control Change
 */
class ControlChangeHandler : public MidiMessageHandler {
public:
    void send(MidiSender* sender, const ComponentConfig& config, uint8_t value, uint16_t raw_value = 0) override {
        if (!sender) return;
        sender->sendControlChange(config.midi_channel, config.midi_param, value);
    }
};

/**
 * @brief Handler pour Program Change
 */
class ProgramChangeHandler : public MidiMessageHandler {
public:
    void send(MidiSender* sender, const ComponentConfig& config, uint8_t value, uint16_t raw_value = 0) override {
        if (!sender) return;
        // Pour Program Change, utiliser config.midi_param (numéro de programme) au lieu de value
        // value est ignoré pour Program Change car il représente toujours 127 depuis ButtonProcessor
        uint8_t program = config.midi_param;
        // Convertir de 1-based (1-128, comme le formulaire) vers 0-based (0-127) pour l'envoi MIDI
        // Cohérent avec le canal MIDI qui est aussi 1-based dans le formulaire et converti à l'envoi
        if (program > 0) {
            program = program - 1; // 1-128 → 0-127
        }
        // S'assurer que le programme est dans la plage valide (0-127)
        if (program > 127) program = 127;
        sender->sendProgramChange(config.midi_channel, program);
    }
};

/**
 * @brief Handler pour Pitch Bend
 */
class PitchBendHandler : public MidiMessageHandler {
public:
    void send(MidiSender* sender, const ComponentConfig& config, uint8_t value, uint16_t raw_value = 0) override {
        if (!sender) return;
        int pitchBend;
        // raw_value est maintenant la valeur brute filtrée (pas normalisée) depuis le processeur
        // Appliquer le mapping potMin/potMax DIRECTEMENT vers 0-16383 (14 bits)
        // Cela garantit que les seuils correspondent exactement aux valeurs min/max du pitchbend
        uint16_t analog_min = 0;
        uint16_t analog_max = 4095;
        
        // Lire les seuils depuis la config spécifique du potentiomètre si disponible
        if (config.type == ComponentType::POTENTIOMETER && config.specificConfig.potentiometer) {
            analog_min = config.specificConfig.potentiometer->potMin;
            analog_max = config.specificConfig.potentiometer->potMax;
            // Si les seuils ne sont pas configurés (0,0), utiliser les valeurs par défaut
            if (analog_min == 0 && analog_max == 0) {
                analog_min = 0;
                analog_max = 4095;
            }
        }
        
        uint16_t bend14bit;
        // Mapping direct : [potMin, potMax] → [0, 16383]
        if (analog_max > analog_min) {
            if (raw_value < analog_min) {
                // En dessous du seuil min → 0 (pitchbend min)
                bend14bit = 0;
            } else if (raw_value > analog_max) {
                // Au dessus du seuil max → 16383 (pitchbend max)
                bend14bit = 16383;
            } else {
                // Entre les seuils → mapper linéairement de [potMin, potMax] vers [0, 16383]
                bend14bit = map(raw_value, analog_min, analog_max, 0, 16383);
            }
        } else {
            // Pas de seuils configurés → mapping direct 0-4095 → 0-16383
            bend14bit = map(raw_value, 0, 4095, 0, 16383);
        }
        
        pitchBend = (int)bend14bit - 8192; // Convertir en signé (-8192 à +8191)
        sender->sendPitchBend(config.midi_channel, pitchBend);
    }
};

/**
 * @brief Handler pour Aftertouch (Channel Pressure)
 */
class AftertouchHandler : public MidiMessageHandler {
public:
    void send(MidiSender* sender, const ComponentConfig& config, uint8_t value, uint16_t raw_value = 0) override {
        if (!sender) return;
        sender->sendAftertouch(config.midi_channel, value);
    }
};

/**
 * @brief Handler pour Note On/Off standard
 */
class NoteHandler : public MidiMessageHandler {
public:
    void send(MidiSender* sender, const ComponentConfig& config, uint8_t value, uint16_t raw_value = 0) override {
        if (!sender) return;
        if (value > 0) {
            sender->sendNoteOn(config.midi_channel, config.midi_param, value);
        } else {
            sender->sendNoteOff(config.midi_channel, config.midi_param, 0);
        }
    }
    
    bool supportsValueRange() const override { return false; } // Note On/Off = discret
};

/**
 * @brief Handler pour Note + Vélocité (vélocité variable)
 */
class NoteVelocityHandler : public MidiMessageHandler {
public:
    void send(MidiSender* sender, const ComponentConfig& config, uint8_t value, uint16_t raw_value = 0) override {
        if (!sender) return;
        if (value > 0) {
            sender->sendNoteOn(config.midi_channel, config.midi_param, value);
        } else {
            sender->sendNoteOff(config.midi_channel, config.midi_param, 0);
        }
    }
};

/**
 * @brief Handler pour Note Sweep (balayage de notes)
 * Nécessite un traitement spécial avec noteMin/Max/VelFix/AutoOffDelay
 */
class NoteSweepHandler : public MidiMessageHandler {
public:
    void send(MidiSender* sender, const ComponentConfig& config, uint8_t value, uint16_t raw_value = 0) override {
        // NOTE: NoteSweep nécessite un traitement spécial qui ne peut pas être fait ici
        // car il nécessite l'état (last_note, note_on_time) et des paramètres spéciaux
        // Ce handler est donc utilisé uniquement pour la détection, le traitement réel
        // doit être fait dans le processeur spécifique
        if (!sender) return;
        // Par défaut, envoyer comme une note normale
        if (value > 0) {
            sender->sendNoteOn(config.midi_channel, config.midi_param, value);
        } else {
            sender->sendNoteOff(config.midi_channel, config.midi_param, 0);
        }
    }
    
    bool requiresSpecialParams() const override { return true; }
};

/**
 * @brief Handler pour Clock
 */
class ClockHandler : public MidiMessageHandler {
public:
    void send(MidiSender* sender, const ComponentConfig& config, uint8_t value, uint16_t raw_value = 0) override {
        (void)config; (void)value; (void)raw_value; // Clock n'utilise pas ces paramètres
        if (!sender) return;
        sender->sendClock();
    }
    
    bool supportsValueRange() const override { return false; }
};

/**
 * @brief Factory pour obtenir le handler approprié selon le type de message
 * Utilise des singletons locaux statiques pour garantir l'initialisation
 */
class MidiMessageHandlerFactory {
public:
    static MidiMessageHandler* getHandler(MidiMessageType msg_type) {
        switch (msg_type) {
            case MidiMessageType::CONTROL_CHANGE: {
                static ControlChangeHandler handler;
                return &handler;
            }
            case MidiMessageType::PROGRAM_CHANGE: {
                static ProgramChangeHandler handler;
                return &handler;
            }
            case MidiMessageType::PITCH_BEND: {
                static PitchBendHandler handler;
                return &handler;
            }
            case MidiMessageType::AFTERTOUCH: {
                static AftertouchHandler handler;
                return &handler;
            }
            case MidiMessageType::NOTE_VELOCITY: {
                static NoteVelocityHandler handler;
                return &handler;
            }
            case MidiMessageType::NOTE_SWEEP: {
                static NoteSweepHandler handler;
                return &handler;
            }
            case MidiMessageType::CLOCK:
            case MidiMessageType::TAP_TEMPO: {
                static ClockHandler handler;
                return &handler;
            }
            case MidiMessageType::NOTE:
            default: {
                static NoteHandler handler;
                return &handler;
            }
        }
    }
};
