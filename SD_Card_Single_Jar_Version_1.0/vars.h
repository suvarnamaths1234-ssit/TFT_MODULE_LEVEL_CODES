#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// enum declarations



// Flow global variables

enum FlowGlobalVariables {
    FLOW_GLOBAL_VARIABLE_JAR1_STATE = 0,
    FLOW_GLOBAL_VARIABLE_JAR1_TIME = 1,
    FLOW_GLOBAL_VARIABLE_MAX_RPM = 2,
    FLOW_GLOBAL_VARIABLE_STEP_RPM = 3,
    FLOW_GLOBAL_VARIABLE_MIN_RPM = 4,
    FLOW_GLOBAL_VARIABLE_MAX_TIME = 5,
    FLOW_GLOBAL_VARIABLE_STEP_TIME = 6
};

// Native global variables

extern int32_t get_var_rpm();
extern void set_var_rpm(int32_t value);
extern int32_t get_var_time();
extern void set_var_time(int32_t value);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/