#pragma once
#include <Arduino.h>

#ifndef NIDMI_NO_C3
extern const char PINCAPS_C3[] PROGMEM;
String buildC3PinCapsJson();
#endif