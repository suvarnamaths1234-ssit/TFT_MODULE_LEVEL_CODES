#include <lvgl.h>
#include <TFT_eSPI.h>
#include "ui.h"
#include "screens.h"
#include <XPT2046_Touchscreen.h>

extern objects_t objects;
static const uint16_t screenWidth = 320;
static const uint16_t screenHeight = 240;

char rpmBuf[8], durBuf[8];

#define TOUCH_CS 21
#define TIRQ_PIN 27

#define motorEnablePin 25
#define motorDirectionPin 26

const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;
const int duty_percentage = 80;

TFT_eSPI tft(screenWidth, screenHeight);
XPT2046_Touchscreen ts(TOUCH_CS, TIRQ_PIN);

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 40];

bool motorOn = false;
int setRPM = 0;              // RPM, 0 = stopped
int setDuration = 0;         // countdown in seconds
bool directionCW = true;
bool jar1_saved = false;

unsigned long motorStartMillis = 0;
unsigned long elapsedMillis = 0;

void update_jar1_state_label(const char *txt, lv_palette_t color) {
    if (objects.jar1_state) {
        lv_label_set_text(objects.jar1_state, txt);
        lv_obj_set_style_text_color(objects.jar1_state, lv_palette_main(color), LV_PART_MAIN);
    }
}

void startMotor() {
    motorOn = true;
    motorStartMillis = millis();
    elapsedMillis = 0;
    int dutyCycle = map(setRPM, 0, 100, 0, 255);
    ledcWrite(pwmChannel, dutyCycle);
    digitalWrite(motorDirectionPin, directionCW ? HIGH : LOW);
    update_jar1_state_label("ON", LV_PALETTE_GREEN);
    //updateUICountdown(setDuration * 1000UL);
}

// Physically stop motor PWM & reset timers & update UI countdown
void stopMotor() {
    Serial.println("Motor Stopped");
    motorOn = false;
     elapsedMillis = 0;
    motorStartMillis = 0;
    ledcWrite(pwmChannel, 0);  
    update_jar1_state_label("OFF", LV_PALETTE_RED);

   // updateUICountdown(0);
}

void updateUICountdown(unsigned long remainingMillis) {
    int secs = remainingMillis / 1000;
    int min = secs / 60;
    int sec = secs % 60;
    char buf[16];
    sprintf(buf, "%02d:%02d", min, sec);
    lv_label_set_text(objects.jar1_time_countdown, buf);
}

void timer_cb(lv_timer_t * timer) {
    unsigned long remainingMillis = 0;

    if (motorOn && setDuration > 0) {
        elapsedMillis = millis() - motorStartMillis;

        if (elapsedMillis >= (unsigned long)(setDuration) * 1000UL) {
            stopMotor();  // This sets motorOn = false and PWM=0

            // Update button labels and colors explicitly for stopped state
            update_button_label(objects.jar_info_start_button, "Start");
            update_button_label(objects.jar_info_stop, "Completed");
            lv_obj_set_style_bg_color(objects.jar_info_start_button, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
            lv_obj_set_style_bg_color(objects.jar_info_stop, lv_palette_main(LV_PALETTE_YELLOW), LV_PART_MAIN);

            remainingMillis = 0;

            Serial.println("Countdown expired — motor stopped, buttons reset");
        } else {
            remainingMillis = (setDuration * 1000UL) - elapsedMillis;
        }
    } else {
        // Motor off: ensure buttons show idle states
        update_button_label(objects.jar_info_start_button, "Start");
        update_button_label(objects.jar_info_stop, "Completed");
        lv_obj_set_style_bg_color(objects.jar_info_start_button, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
        lv_obj_set_style_bg_color(objects.jar_info_stop, lv_palette_main(LV_PALETTE_YELLOW), LV_PART_MAIN);
    }

    updateUICountdown(remainingMillis);
    updateTimeLeftReadings(remainingMillis);
}

void updateRpmReadings() {
    char buf[16];
    sprintf(buf, "RPM: %d", setRPM);
    lv_label_set_text(objects.readings_rpm, buf);
}

void updateTimeLeftReadings(unsigned long remainingMillis) {
    int secs = remainingMillis / 1000;
    int min = secs / 60;
    int sec = secs % 60;
    char buf[24];
    sprintf(buf, "Time left: %02d:%02d", min, sec);
    lv_label_set_text(objects.readings_timeleft, buf);
}

void updateUIFields() {
    //char rpmBuf[8], durBuf[8];
    sprintf(rpmBuf, "%d", setRPM);
    sprintf(durBuf, "%d", setDuration);
    lv_textarea_set_text(objects.rpm_textarea, rpmBuf);
    lv_textarea_set_text(objects.duration_textarea, durBuf);
    updateRpmReadings();
}


void rpm_plus_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        setRPM += 5;
        if (setRPM > 100) setRPM = 100;
        jar1_saved = false;     // Invalidate save
        lv_obj_set_style_bg_color(objects.jar_info_save, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
        if (motorOn) {
            ledcWrite(pwmChannel, map(setRPM, 0, 100, 0, 255));
        }
        updateUIFields();
    }
}

void rpm_minus_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        setRPM -= 5;
        if (setRPM < 0) setRPM = 0;
        jar1_saved = false;     // Invalidate save
        lv_obj_set_style_bg_color(objects.jar_info_save, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
        if (motorOn && setRPM > 0) {
            ledcWrite(pwmChannel, map(setRPM, 0, 100, 0, 255));
        } else if (motorOn && setRPM == 0) {
            stopMotor();
        }
        updateUIFields();
    }
}

void time_plus_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        setDuration += 10;
        if (setDuration > 3600) setDuration = 3600;
        jar1_saved = false; // invalidate save when time changes
        lv_obj_set_style_bg_color(objects.jar_info_save, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
        updateUIFields();
    }
}

void time_minus_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        setDuration -= 10;
        if (setDuration < 0) setDuration = 0;
        jar1_saved = false; // invalidate save when time changes
        lv_obj_set_style_bg_color(objects.jar_info_save, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
        updateUIFields();
    }
}

void direction_switch_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        directionCW = lv_obj_has_state(objects.config_motor_rotation_direction_switch, LV_STATE_CHECKED);
        Serial.print("Direction set to: ");
        Serial.println(directionCW ? "Clockwise" : "Anti-Clockwise");
        if (motorOn) {
            digitalWrite(motorDirectionPin, directionCW ? HIGH : LOW);
        }
    }
}
void start_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        if (!jar1_saved) {  // jar1_saved is a global flag you add
            Serial.println("Please save settings before starting motor.");
            return; // Prevent motor start unless saved
        }
        if (setRPM > 0 && setDuration > 0) {
            startMotor();
            updateUIFields();
  } else {
           
            Serial.println("Set RPM and Duration before starting motor");
        }
    }
}


void stop_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        stopMotor();
        updateUIFields();
        updateUICountdown(0);
    }
}
// Save button callback sets the flag and updates Save button color
void jar1_save_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        jar1_saved = true;
        Serial.print("Jar1 settings saved: RPM=");
        Serial.print(setRPM);
        Serial.print(", Duration=");
        Serial.println(setDuration);
        Serial.println("Jar1 settings saved.");

        lv_obj_set_style_bg_color(objects.jar_info_save, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(objects.jar_info_save, LV_OPA_COVER, LV_PART_MAIN);
        
    }
}


void update_button_label(lv_obj_t *btn, const char *txt) {
    if (btn) {
        lv_obj_t *label = lv_obj_get_child(btn, 0);
        if (label) {
            lv_label_set_text(label, txt);
        }
    }
}

void jar1_start_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        if (!jar1_saved) {
            Serial.println("Please save settings before starting motor.");
            return;
        }
        if (setRPM > 0 && setDuration > 0) {
            startMotor();
            updateUIFields();

            // Button labels and colors indicating running motor
            update_button_label(objects.jar_info_start_button, "Running");
            update_button_label(objects.jar_info_stop, "Stop");

            lv_obj_set_style_bg_color(objects.jar_info_start_button, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_MAIN);
            lv_obj_set_style_bg_color(objects.jar_info_stop, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);

            Serial.println("Motor started, UI updated");
        } else {
            Serial.println("Set RPM and Duration before starting motor");
        }
    }
}


void jar1_stop_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        stopMotor();
        updateUIFields();
        updateUICountdown(0);

        update_button_label(objects.jar_info_stop, "Completed");
        update_button_label(objects.jar_info_start_button, "Start");

        lv_obj_set_style_bg_color(objects.jar_info_stop, lv_palette_main(LV_PALETTE_YELLOW), LV_PART_MAIN);
        lv_obj_set_style_bg_color(objects.jar_info_start_button, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);

        Serial.println("Motor stopped - UI updated");
    }
}

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)color_p, w * h, true);
    tft.endWrite();
    lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    if (!ts.touched()) {
        data->state = LV_INDEV_STATE_REL;
    } else {
        TS_Point p = ts.getPoint();
        int x = map(p.x, 644, 3776, 0, screenWidth);
        int y = map(p.y, 398, 3496, 0, screenHeight);
        x = constrain(x, 0, screenWidth - 1);
        y = constrain(y, 0, screenHeight - 1);
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PR;
    }
}
void setup() {
    Serial.begin(115200);
    pinMode(motorDirectionPin, OUTPUT);
    digitalWrite(motorDirectionPin, LOW);

        ledcAttach(motorEnablePin, freq, resolution);


    lv_init();
    tft.begin();
    tft.setRotation(3);
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 40);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    ts.begin();
    ts.setRotation(1);

    ui_init();
    updateUIFields();

    lv_obj_add_event_cb(objects.info_page_rpm_increase_icon, rpm_plus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.info_page_rpm_decrease_icon, rpm_minus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.info_page_time_increase_icon, time_plus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.info_page_time_decrease_icon, time_minus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.config_motor_rotation_direction_switch, direction_switch_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(objects.jar_info_start_button, start_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.jar_info_stop, stop_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.jar_info_start_button, jar1_start_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.jar_info_stop, jar1_stop_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.jar_info_save, jar1_save_cb, LV_EVENT_CLICKED, NULL);

    lv_timer_create(timer_cb, 1000, NULL);
}

void loop() {
    int dutyCycle = (duty_percentage * 255) / 100;
    ledcWrite(motorEnablePin, motorOn ? dutyCycle : 0);

    lv_tick_inc(5);
    lv_timer_handler();
    ui_tick();
    delay(5);
}

