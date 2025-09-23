#include "pincaps_c3.h"

#ifndef ESP32SERVER_NO_C3
const char PINCAPS_C3[] PROGMEM = R"JSON({
  "board":"xiao-esp32c3",
  "pins":[
    {"gpio":2, "label":"D0", "caps":{"in":true,"out":true,"adc":true,"pwm":true,"touch":false}, "sensitive":false},
    {"gpio":3, "label":"D1", "caps":{"in":true,"out":true,"adc":true,"pwm":true,"touch":false}, "sensitive":false},
    {"gpio":4, "label":"D2", "caps":{"in":true,"out":true,"adc":true,"pwm":true,"touch":false}, "sensitive":false},
    {"gpio":5, "label":"D3", "caps":{"in":true,"out":true,"adc":true,"pwm":true,"touch":false}, "sensitive":false},
    {"gpio":6, "label":"D4", "caps":{"in":true,"out":true,"adc":false,"pwm":true,"touch":false}, "sensitive":false},
    {"gpio":7, "label":"D5", "caps":{"in":true,"out":true,"adc":false,"pwm":true,"touch":false}, "sensitive":false},
    {"gpio":21, "label":"D6", "caps":{"in":true,"out":true,"adc":false,"pwm":true,"touch":false}, "sensitive":false},
    {"gpio":20, "label":"D7", "caps":{"in":true,"out":true,"adc":false,"pwm":true,"touch":false}, "sensitive":false},
    {"gpio":8, "label":"D8", "caps":{"in":true,"out":true,"adc":false,"pwm":true,"touch":false}, "sensitive":false},
    {"gpio":9, "label":"D9", "caps":{"in":true,"out":true,"adc":false,"pwm":true,"touch":false}, "sensitive":false},
    {"gpio":10, "label":"D10", "caps":{"in":true,"out":true,"adc":false,"pwm":true,"touch":false}, "sensitive":false}
  ],
  "bus":{
    "i2c":{"sda":6,"scl":7},
    "spi":{"mosi":10,"miso":9,"sck":8},
    "uart":{"tx":21,"rx":20}
  }
})JSON";

String buildC3PinCapsJson() {
    return String(PINCAPS_C3);
}
#endif