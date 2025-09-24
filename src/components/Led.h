#pragma once

#include <Arduino.h>

class Led {
public:
    explicit Led(uint8_t pin) : pin(pin), pwm(false) {}

    void begin(bool usePwm = false) {
        pwm = usePwm;
        pinMode(pin, OUTPUT);
        if (!pwm) digitalWrite(pin, LOW);
    }

    void set(bool on) { if (!pwm) digitalWrite(pin, on ? HIGH : LOW); }

    void setBrightness(uint8_t value) {
        if (!pwm) { digitalWrite(pin, value > 0 ? HIGH : LOW); return; }
        // Placeholder: implémentation PWM selon core (LEDC)
        // ledcWrite(channel, value * 2); // 0-255 → 0-510 (~10 bits)
    }

private:
    uint8_t pin;
    bool pwm;
};


