#include "pincaps_s3.h"

#ifndef ESP32SERVER_NO_S3
const char PINCAPS_S3[] PROGMEM = R"JSON({
  "board":"xiao-esp32s3",
  "pins":[
    {"gpio":1,  "label":"D0", "caps":{"in":true,"out":true,"adc":true,"pwm":true,"touch":true},  "sensitive":false},
    {"gpio":2,  "label":"D1", "caps":{"in":true,"out":true,"adc":true,"pwm":true,"touch":true},  "sensitive":false},
    {"gpio":3,  "label":"D2", "caps":{"in":true,"out":true,"adc":true,"pwm":true,"touch":true},  "sensitive":false},
    {"gpio":4,  "label":"D3", "caps":{"in":true,"out":true,"adc":true,"pwm":true,"touch":true},  "sensitive":false},
    {"gpio":5,  "label":"D4", "caps":{"in":true,"out":true,"adc":true,"pwm":true,"touch":true},  "sensitive":false},
    {"gpio":6,  "label":"D5", "caps":{"in":true,"out":true,"adc":false,"pwm":true,"touch":true}, "sensitive":false},
    {"gpio":7,  "label":"D6", "caps":{"in":true,"out":true,"adc":false,"pwm":true,"touch":true}, "sensitive":false},
    {"gpio":8,  "label":"D7", "caps":{"in":true,"out":true,"adc":false,"pwm":true,"touch":true}, "sensitive":false},
    {"gpio":9,  "label":"D8", "caps":{"in":true,"out":true,"adc":false,"pwm":true,"touch":true}, "sensitive":false},
    {"gpio":10, "label":"D9", "caps":{"in":true,"out":true,"adc":false,"pwm":true,"touch":true}, "sensitive":false}
  ],
  "bus":{
    "i2c":{"sda":4,"scl":5},
    "spi":{"mosi":7,"miso":6,"sck":8},
    "uart":{"tx":43,"rx":44}
  }
})JSON";
#endif
