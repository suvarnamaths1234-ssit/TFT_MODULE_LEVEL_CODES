#include <stdint.h>
#include <Arduino.h>
#include "vars.h"

static int32_t var_rpm = 0;
static int32_t var_time = 0;


extern "C" int32_t get_var_rpm() {
    return var_rpm;
}

extern "C" void set_var_rpm(int32_t value) {
    var_rpm = value;
    //Serial.print("RPM set to: ");
    //Serial.println(var_rpm);
}

extern "C" int32_t get_var_time() {
    return var_time;
}

extern "C" void set_var_time(int32_t value) {
    var_time = value;
   // Serial.print("Time set to: ");
    //Serial.println(var_time);
}
