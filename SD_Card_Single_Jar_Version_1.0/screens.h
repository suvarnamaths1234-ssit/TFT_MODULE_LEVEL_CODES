#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *jar_info_page;
    lv_obj_t *errors_page;
    lv_obj_t *jar_configuration_page;
    lv_obj_t *settings_page;
    lv_obj_t *settings_button;
    lv_obj_t *errors_button;
    lv_obj_t *jar1_button;
    lv_obj_t *custom_configuration_button;
    lv_obj_t *obj0;
    lv_obj_t *info_page_rpm_decrease_icon;
    lv_obj_t *info_page_rpm_increase_icon;
    lv_obj_t *info_page_time_decrease_icon;
    lv_obj_t *info_page_time_increase_icon;
    lv_obj_t *obj1;
    lv_obj_t *home_button;
    lv_obj_t *rpm_decrease_icon;
    lv_obj_t *rpm_increase_icon;
    lv_obj_t *time_decrease_icon;
    lv_obj_t *time_increase_icon;
    lv_obj_t *obj2;
    lv_obj_t *time_home_page;
    lv_obj_t *project_name;
    lv_obj_t *power_button;
    lv_obj_t *obj3;
    lv_obj_t *obj4;
    lv_obj_t *jar1_time_countdown;
    lv_obj_t *jar2_button;
    lv_obj_t *jar3_button;
    lv_obj_t *jar4_button;
    lv_obj_t *jar5_button;
    lv_obj_t *jar6_button;
    lv_obj_t *jar7_button;
    lv_obj_t *jar8_button;
    lv_obj_t *jar1_state;
    lv_obj_t *start_all_button_1;
    lv_obj_t *stop_all_button_1;
    lv_obj_t *jar2_time_countdown;
    lv_obj_t *jar2_state;
    lv_obj_t *jar4_time_countdown;
    lv_obj_t *jar4_state;
    lv_obj_t *jar3_time_countdown;
    lv_obj_t *jar3_state;
    lv_obj_t *jar5_time_countdown;
    lv_obj_t *jar5_state;
    lv_obj_t *jar6_time_countdown;
    lv_obj_t *jar6_state;
    lv_obj_t *jar7_time_countdown;
    lv_obj_t *jar7_state;
    lv_obj_t *jar8_time_countdown;
    lv_obj_t *jar8_state;
    lv_obj_t *obj5;
    lv_obj_t *obj6;
    lv_obj_t *obj7;
    lv_obj_t *jar_info_save;
    lv_obj_t *jar_info_start_button;
    lv_obj_t *jar_info_stop;
    lv_obj_t *readings_title_name;
    lv_obj_t *readings_timeleft;
    lv_obj_t *readings_rpm;
    lv_obj_t *readings_temp;
    lv_obj_t *readings_current;
    lv_obj_t *readings_voltage;
    lv_obj_t *obj8;
    lv_obj_t *info_page_jar_direction_icon;
    lv_obj_t *obj9;
    lv_obj_t *obj10;
    lv_obj_t *obj11;
    lv_obj_t *jar1_checkbox;
    lv_obj_t *jar2_checkbox;
    lv_obj_t *jar3_checkbox;
    lv_obj_t *jar4_checkbox;
    lv_obj_t *jar5_checkbox;
    lv_obj_t *jar6_checkbox;
    lv_obj_t *jar8_checkbox;
    lv_obj_t *jar7_checkbox;
    lv_obj_t *obj12;
    lv_obj_t *obj13;
    lv_obj_t *rpm_textarea;
    lv_obj_t *obj14;
    lv_obj_t *duration_textarea;
    lv_obj_t *all_jar_configurations_save_button;
    lv_obj_t *start_all_button;
    lv_obj_t *stop_all_button;
    lv_obj_t *obj15;
    lv_obj_t *obj16;
    lv_obj_t *config_motor_rotation_direction_switch;
    lv_obj_t *obj17;
    lv_obj_t *obj18;
    lv_obj_t *obj19;
    lv_obj_t *obj20;
    lv_obj_t *obj21;
    lv_obj_t *obj22;
    lv_obj_t *obj23;
    lv_obj_t *obj24;
    lv_obj_t *obj25;
    lv_obj_t *obj26;
    lv_obj_t *obj27;
    lv_obj_t *obj28;
    lv_obj_t *time_lbl;
    lv_obj_t  *Set_RPM;
    lv_obj_t  *RPM;
    lv_obj_t  *jar1_motor_on;
    lv_obj_t  *jar1_motor_off;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_MAIN = 1,
    SCREEN_ID_JAR_INFO_PAGE = 2,
    SCREEN_ID_ERRORS_PAGE = 3,
    SCREEN_ID_JAR_CONFIGURATION_PAGE = 4,
    SCREEN_ID_SETTINGS_PAGE = 5,
};

void create_screen_main();
void tick_screen_main();

void create_screen_jar_info_page();
void tick_screen_jar_info_page();

void create_screen_errors_page();
void tick_screen_errors_page();

void create_screen_jar_configuration_page();
void tick_screen_jar_configuration_page();

void create_screen_settings_page();
void tick_screen_settings_page();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/