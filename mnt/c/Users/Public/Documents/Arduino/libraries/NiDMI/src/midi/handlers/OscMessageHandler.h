#pragma once

#include <Arduino.h>
#include "../../components/ComponentTypes.h"
#include "../../osc/OSCQueue.h"
#include "../MidiMessageType.h"

/**
 * @file OscMessageHandler.h
 * @brief Handlers spécialisés pour l'envoi de messages OSC
 * 
 * Supporte différents formats : RAW (0-4095), MIDI (3 int), Float (0-1)
 */

/**
 * @brief Interface de base pour les handlers OSC
 */
class OscMessageHandler {
public:
    virtual ~OscMessageHandler() {}
    
    /**
     * @brief Envoie un message OSC selon le format du handler
     * 
     * @param osc_queue Queue OSC
     * @param config Configuration du composant
     * @param midi_value Valeur MIDI (0-127)
     * @param raw_value Valeur brute (0-4095) pour format RAW
     */
    virtual void send(OSCQueue& osc_queue, const ComponentConfig& config, 
                     uint8_t midi_value, uint16_t raw_value = 0) = 0;
};

/**
 * @brief Handler pour format RAW (valeur brute 0-4095)
 */
class OscRawHandler : public OscMessageHandler {
public:
    void send(OSCQueue& osc_queue, const ComponentConfig& config, 
             uint8_t midi_value, uint16_t raw_value) override {
        (void)midi_value; // Non utilisé pour RAW
        String oscAddress = (config.osc_address[0] != '\0') 
            ? String(config.osc_address) 
            : getDefaultAddress(config.msg_type);
        uint16_t raw_array[1] = {raw_value};
        osc_queue.enqueueIntArray(oscAddress, raw_array, 1);
    }

private:
    String getDefaultAddress(MidiMessageType msg_type) {
        switch (msg_type) {
            case MidiMessageType::NOTE:
            case MidiMessageType::NOTE_VELOCITY:
            case MidiMessageType::NOTE_SWEEP:
                return "/note";
            case MidiMessageType::CONTROL_CHANGE:
                return "/ctl";
            case MidiMessageType::PROGRAM_CHANGE:
                return "/pc";
            default:
                return "/ctl";
        }
    }
};

/**
 * @brief Handler pour format MIDI (3 int: value, param, channel)
 */
class OscMidiHandler : public OscMessageHandler {
public:
    void send(OSCQueue& osc_queue, const ComponentConfig& config, 
             uint8_t midi_value, uint16_t raw_value) override {
        (void)raw_value; // Non utilisé pour MIDI
        String oscAddress = (config.osc_address[0] != '\0') 
            ? String(config.osc_address) 
            : getDefaultAddress(config.msg_type);
        osc_queue.enqueueMidi(oscAddress, midi_value, config.midi_param, config.midi_channel);
    }

private:
    String getDefaultAddress(MidiMessageType msg_type) {
        switch (msg_type) {
            case MidiMessageType::NOTE:
            case MidiMessageType::NOTE_VELOCITY:
            case MidiMessageType::NOTE_SWEEP:
                return "/note";
            case MidiMessageType::CONTROL_CHANGE:
                return "/ctl";
            case MidiMessageType::PROGRAM_CHANGE:
                return "/pc";
            default:
                return "/ctl";
        }
    }
};

/**
 * @brief Handler pour format Float (valeur normalisée 0-1)
 */
class OscFloatHandler : public OscMessageHandler {
public:
    void send(OSCQueue& osc_queue, const ComponentConfig& config, 
             uint8_t midi_value, uint16_t raw_value) override {
        (void)raw_value; // Non utilisé pour Float
        String oscAddress = (config.osc_address[0] != '\0') 
            ? String(config.osc_address) 
            : getDefaultAddress(config.msg_type);
        osc_queue.enqueueFloat(oscAddress, midi_value / 127.0f);
    }

private:
    String getDefaultAddress(MidiMessageType msg_type) {
        switch (msg_type) {
            case MidiMessageType::NOTE:
            case MidiMessageType::NOTE_VELOCITY:
            case MidiMessageType::NOTE_SWEEP:
                return "/note";
            case MidiMessageType::CONTROL_CHANGE:
                return "/ctl";
            case MidiMessageType::PROGRAM_CHANGE:
                return "/pc";
            default:
                return "/ctl";
        }
    }
};

/**
 * @brief Factory pour obtenir le handler OSC approprié selon le format
 * Utilise des singletons locaux statiques pour garantir l'initialisation
 */
class OscMessageHandlerFactory {
public:
    static OscMessageHandler* getHandler(uint8_t flags) {
        if (flags & 0x08) { // Format RAW
            static OscRawHandler handler;
            return &handler;
        } else if (flags & 0x04) { // Format MIDI
            static OscMidiHandler handler;
            return &handler;
        } else { // Format Float (défaut)
            static OscFloatHandler handler;
            return &handler;
        }
    }
};
