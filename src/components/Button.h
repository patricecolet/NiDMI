#pragma once

#include <Arduino.h>
#include "../midi/MidiSender.h"

/**
 * @brief Anti-rebond temporel sur entrée digitale.
 */
class Debounce {
public:
    explicit Debounce(uint32_t delayMs = 25)
        : delayMs(delayMs), lastChange(0), stableState(false), lastRead(false) {}

    bool process(bool read) {
        uint32_t now = millis();
        if (read != lastRead) { lastChange = now; lastRead = read; }
        if (now - lastChange >= delayMs) { stableState = lastRead; }
        return stableState;
    }

private:
    uint32_t delayMs;
    uint32_t lastChange;
    bool stableState;
    bool lastRead;
};

/**
 * @brief Bouton mappé en Note On/Off via un émetteur (`MidiSender`).
 *
 * Câblage par défaut: INPUT_PULLUP, bouton vers GND (actif à LOW).
 * Exemple: Button(D2, 60, 1, router) enverra Note 60 sur canal 1.
 */
class Button {
public:
    /**
     * @param pin      Broche digitale (ex: D2)
     * @param note     Numéro de note (0-127)
     * @param channel  Canal MIDI (1-16)
     * @param out      Routeur/émetteur MIDI (RTP-MIDI/OSC)
     */
    Button(uint8_t pin, uint8_t note, uint8_t channel, MidiSender& out)
        : pin(pin), note(note), channel(channel), out(out), state(false) {}

    void begin() { pinMode(pin, INPUT_PULLUP); }

    void update() {
        bool pressed = !digitalRead(pin);
        bool debounced = deb.process(pressed);
        if (debounced != state) {
            state = debounced;
            if (state) out.sendNoteOn(channel, note, 127);
            else out.sendNoteOff(channel, note, 0);
        }
    }

private:
    uint8_t pin;
    uint8_t note;
    uint8_t channel;
    MidiSender& out;
    Debounce deb;
    bool state;
};


