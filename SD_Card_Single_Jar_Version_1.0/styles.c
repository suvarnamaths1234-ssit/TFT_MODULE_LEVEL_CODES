#include "styles.h"
#include "images.h"
#include "fonts.h"

#include "ui.h"
#include "screens.h"

//
// Style: style_idle
//

void add_style_style_idle(lv_obj_t *obj) {
    (void)obj;
};

void remove_style_style_idle(lv_obj_t *obj) {
    (void)obj;
};

//
// Style: style_running
//

void init_style_style_running_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(0xff14e914));
};

lv_style_t *get_style_style_running_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = lv_mem_alloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_style_running_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_style_running(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_style_running_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_style_running(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_style_running_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
// Style: style_stopped
//

void init_style_style_stopped_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(0xfff32121));
};

lv_style_t *get_style_style_stopped_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = lv_mem_alloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_style_stopped_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_style_stopped(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_style_stopped_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_style_stopped(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_style_stopped_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
//
//

void add_style(lv_obj_t *obj, int32_t styleIndex) {
    typedef void (*AddStyleFunc)(lv_obj_t *obj);
    static const AddStyleFunc add_style_funcs[] = {
        add_style_style_idle,
        add_style_style_running,
        add_style_style_stopped,
    };
    add_style_funcs[styleIndex](obj);
}

void remove_style(lv_obj_t *obj, int32_t styleIndex) {
    typedef void (*RemoveStyleFunc)(lv_obj_t *obj);
    static const RemoveStyleFunc remove_style_funcs[] = {
        remove_style_style_idle,
        remove_style_style_running,
        remove_style_style_stopped,
    };
    remove_style_funcs[styleIndex](obj);
}

