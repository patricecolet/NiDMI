#pragma once
#include <Arduino.h>

#ifndef ESP32SERVER_NO_C3
extern const char PINCAPS_C3[] PROGMEM;
String buildC3PinCapsJson();
#endif