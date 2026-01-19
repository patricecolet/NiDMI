#pragma once

#include <Arduino.h>
#include "../../components/ComponentTypes.h"
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
     */
    virtual void send(MidiSender* sender, const ComponentConfig& config, uint8_t value) = 0;
    
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
    void send(MidiSender* sender, const ComponentConfig& config, uint8_t value) override {
        if (!sender) return;
        sender->sendControlChange(config.midi_channel, config.midi_param, value);
    }
};

/**
 * @brief Handler pour Program Change
 */
class ProgramChangeHandler : public MidiMessageHandler {
public:
    void send(MidiSender* sender, const ComponentConfig& config, uint8_t value) override {
        if (!sender) return;
        sender->sendProgramChange(config.midi_channel, value);
    }
};

/**
 * @brief Handler pour Pitch Bend
 */
class PitchBendHandler : public MidiMessageHandler {
public:
    void send(MidiSender* sender, const ComponentConfig& config, uint8_t value) override {
        if (!sender) return;
        // Conversion: 0-127 → -8192 à +8191 (signé, centre=0)
        int pitchBend = map(value, 0, 127, -8192, 8191);
        sender->sendPitchBend(config.midi_channel, pitchBend);
    }
};

/**
 * @brief Handler pour Aftertouch (Channel Pressure)
 */
class AftertouchHandler : public MidiMessageHandler {
public:
    void send(MidiSender* sender, const ComponentConfig& config, uint8_t value) override {
        if (!sender) return;
        sender->sendAftertouch(config.midi_channel, value);
    }
};

/**
 * @brief Handler pour Note On/Off standard
 */
class NoteHandler : public MidiMessageHandler {
public:
    void send(MidiSender* sender, const ComponentConfig& config, uint8_t value) override {
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
    void send(MidiSender* sender, const ComponentConfig& config, uint8_t value) override {
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
    void send(MidiSender* sender, const ComponentConfig& config, uint8_t value) override {
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
    void send(MidiSender* sender, const ComponentConfig& config, uint8_t value) override {
        (void)config; (void)value; // Clock n'utilise pas ces paramètres
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
