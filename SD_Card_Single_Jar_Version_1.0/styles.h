#ifndef EEZ_LVGL_UI_STYLES_H
#define EEZ_LVGL_UI_STYLES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Style: style_idle
void add_style_style_idle(lv_obj_t *obj);
void remove_style_style_idle(lv_obj_t *obj);

// Style: style_running
lv_style_t *get_style_style_running_MAIN_DEFAULT();
void add_style_style_running(lv_obj_t *obj);
void remove_style_style_running(lv_obj_t *obj);

// Style: style_stopped
lv_style_t *get_style_style_stopped_MAIN_DEFAULT();
void add_style_style_stopped(lv_obj_t *obj);
void remove_style_style_stopped(lv_obj_t *obj);



#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_STYLES_H*/