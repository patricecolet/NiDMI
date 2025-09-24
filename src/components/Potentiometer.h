#pragma once

#include <Arduino.h>
#include "../midi/MidiSender.h"
#include "../midi/messages.h"

/**
 * @brief Filtre passe-bas simple avec hystérésis pour lecture analogique.
 */
class AnalogFilter {
public:
    AnalogFilter(float alpha = 0.12f, uint16_t hysteresis = 2)
        : alpha(alpha), hysteresis(hysteresis), initialized(false), filtered(0) {}

    uint16_t process(uint16_t raw) {
        if (!initialized) { filtered = raw; initialized = true; return raw; }
        float f = alpha * raw + (1.0f - alpha) * filtered;
        if (abs((int)f - (int)filtered) < hysteresis) return (uint16_t)filtered;
        filtered = f; return (uint16_t)filtered;
    }

private:
    float alpha;
    uint16_t hysteresis;
    bool initialized;
    float filtered;
};

/**
 * @brief Potentiomètre mappé en CC MIDI via un émetteur (`MidiSender`).
 *
 * Exemple: Potentiometer(A0, 7, 1, router) enverra CC 7 sur le canal 1.
 */
class Potentiometer {
public:
    /**
     * @param pin      Broche analogique (ex: A0)
     * @param cc       Numéro de Control Change (0-127)
     * @param channel  Canal MIDI (1-16)
     * @param out      Routeur/émetteur MIDI (RTP-MIDI/OSC)
     */
    Potentiometer(uint8_t pin, uint8_t cc, uint8_t channel, MidiSender& out)
        : pin(pin), cc(cc), channel(channel), out(out), lastValue(255) {}

    void begin() { pinMode(pin, INPUT); }

    void update() {
        uint16_t raw = analogRead(pin);
        uint16_t filt = filter.process(raw);
        uint8_t v = map(filt, 0, 4095, 0, 127);
        if (v != lastValue) {
            out.sendControlChange(channel, cc, v);
            lastValue = v;
        }
    }

private:
    uint8_t pin;
    uint8_t cc;
    uint8_t channel;
    MidiSender& out;
    AnalogFilter filter;
    uint8_t lastValue;
};


