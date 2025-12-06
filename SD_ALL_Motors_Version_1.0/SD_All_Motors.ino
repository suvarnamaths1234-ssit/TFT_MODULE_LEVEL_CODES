#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "ui.h"
#include "vars.h"
#include "screens.h"
#include "JarController.h"

extern objects_t objects;

// ==================== Display Configuration ====================
static const uint16_t screenWidth = 320;
static const uint16_t screenHeight = 240;

// ==================== Pin Definitions ====================
#define TOUCH_CS 21
#define TIRQ_PIN 27

// Motor pins for 8 Jars (UPDATE WITH YOUR ACTUAL PINS!)
#define JAR1_ENABLE_PIN 25
#define JAR1_DIRECTION_PIN 26
#define JAR2_ENABLE_PIN 32
#define JAR2_DIRECTION_PIN 33
#define JAR3_ENABLE_PIN 27
#define JAR3_DIRECTION_PIN 14
#define JAR4_ENABLE_PIN 12
#define JAR4_DIRECTION_PIN 13
#define JAR5_ENABLE_PIN 15
#define JAR5_DIRECTION_PIN 2
#define JAR6_ENABLE_PIN 4
#define JAR6_DIRECTION_PIN 16
#define JAR7_ENABLE_PIN 17
#define JAR7_DIRECTION_PIN 5
#define JAR8_ENABLE_PIN 18
#define JAR8_DIRECTION_PIN 19

// SPI Pin Configuration for SD Card
const int sck = 14;
const int miso = 12;
const int mosi = 13;
const int cs = 5;

// ==================== Hardware Objects ====================
TFT_eSPI tft(screenWidth, screenHeight);
XPT2046_Touchscreen ts(TOUCH_CS, TIRQ_PIN);
SPIClass spiSD(HSPI);  // Separate SPI for SD card

// ==================== LVGL Display Buffer ====================
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 40];

// ==================== Global State ====================
bool sdCardReady = false;

// ==================== Jar Controllers Array ====================
JarController* jars[8];
JarController* currentJar = nullptr;  // Currently selected jar
int currentJarIndex = -1;

// ==================== SD Card Helper Functions ====================
void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
    Serial.printf("Listing directory: %s\n", dirname);
    
    File root = fs.open(dirname);
    if (!root) {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        Serial.println("Not a directory");
        root.close();
        return;
    }
    
    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels) {
                listDir(fs, file.path(), levels - 1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
    
    root.close();
}

// ==================== Helper Function to Update UI for Current Jar ====================
void updateUIForCurrentJar() {
    if (currentJar != nullptr) {
        currentJar->updateUIFields();
        currentJar->updateUICountdown();
        
        // Update jar title/label to show which jar is selected
        char jarTitle[32];
        sprintf(jarTitle, "JAR %d", currentJarIndex + 1);
        // If you have a title label, update it here:
        // lv_label_set_text(objects.jar_title_label, jarTitle);
        
        Serial.printf("Switched to Jar %d\n", currentJarIndex + 1);
    }
}

// ==================== Jar Selection Callbacks (Home Screen Buttons) ====================
void select_jar_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        int jarIndex = (int)(intptr_t)lv_event_get_user_data(e);
        currentJarIndex = jarIndex;
        currentJar = jars[jarIndex];
        
        Serial.printf("Selected Jar %d\n", jarIndex + 1);
        
        // Navigate to info screen
        // lv_scr_load(objects.info_screen);  // Uncomment and use your screen object
        
        // Update UI to show current jar's data
        updateUIForCurrentJar();
    }
}

// ==================== Event Callbacks for Shared Info Screen ====================

void rpm_textarea_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_READY && currentJar != nullptr) {
        const char *txt = lv_textarea_get_text(objects.rpm_textarea);
        currentJar->setRPMValue(atoi(txt));
        currentJar->updateUIFields();
    }
}

void duration_textarea_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_READY && currentJar != nullptr) {
        const char *txt = lv_textarea_get_text(objects.duration_textarea);
        currentJar->setDurationValue(atoi(txt));
        currentJar->updateUIFields();
    }
}

void rpm_plus_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED && currentJar != nullptr) {
        currentJar->incrementRPM(5);
    }
}

void rpm_minus_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED && currentJar != nullptr) {
        currentJar->decrementRPM(5);
    }
}

void time_plus_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED && currentJar != nullptr) {
        currentJar->incrementDuration(10);
    }
}

void time_minus_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED && currentJar != nullptr) {
        currentJar->decrementDuration(10);
    }
}

void direction_switch_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED && currentJar != nullptr) {
        bool cw = lv_obj_has_state(objects.config_motor_rotation_direction_switch, LV_STATE_CHECKED);
        currentJar->setDirection(cw);
        Serial.printf("Jar %d Direction: %s\n", currentJarIndex + 1, cw ? "CW" : "CCW");
    }
}

void save_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED && currentJar != nullptr) {
        Serial.printf("=== Jar %d Save Button Clicked ===\n", currentJarIndex + 1);
        
        if (sdCardReady) {
            bool success = currentJar->saveSettings("/jar_settings.txt");
            if (success) {
                Serial.printf("Jar %d: Settings saved successfully\n", currentJarIndex + 1);
            } else {
                Serial.printf("Jar %d: Failed to save settings\n", currentJarIndex + 1);
            }
        } else {
            Serial.println("ERROR: SD card not ready");
        }
    }
}

void start_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED && currentJar != nullptr) {
        if (!currentJar->areSettingsSaved()) {
            Serial.printf("Jar %d: Please save settings before starting\n", currentJarIndex + 1);
            // Flash save button red
            if (objects.jar_info_save) {
                lv_obj_set_style_bg_color(objects.jar_info_save, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
            }
            return;
        }
        
        currentJar->startMotor();
    }
}

void stop_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED && currentJar != nullptr) {
        currentJar->stopMotor();
    }
}

// ==================== Timer Callback ====================
void timer_cb(lv_timer_t *timer) {
    // Update all jars
    for (int i = 0; i < 8; i++) {
        jars[i]->onTimerTick();
    }
    
    // If we're viewing a jar, update its UI
    if (currentJar != nullptr) {
        currentJar->updateUICountdown();
    }
}

// ==================== Display Driver Functions ====================
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

// ==================== Setup ====================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== 8-Jar Motor Controller Starting ===");
    
    // Initialize all 8 Jar Controllers
    jars[0] = new JarController(1, JAR1_ENABLE_PIN, JAR1_DIRECTION_PIN, 0);
    jars[1] = new JarController(2, JAR2_ENABLE_PIN, JAR2_DIRECTION_PIN, 1);
    jars[2] = new JarController(3, JAR3_ENABLE_PIN, JAR3_DIRECTION_PIN, 2);
    jars[3] = new JarController(4, JAR4_ENABLE_PIN, JAR4_DIRECTION_PIN, 3);
    jars[4] = new JarController(5, JAR5_ENABLE_PIN, JAR5_DIRECTION_PIN, 4);
    jars[5] = new JarController(6, JAR6_ENABLE_PIN, JAR6_DIRECTION_PIN, 5);
    jars[6] = new JarController(7, JAR7_ENABLE_PIN, JAR7_DIRECTION_PIN, 6);
    jars[7] = new JarController(8, JAR8_ENABLE_PIN, JAR8_DIRECTION_PIN, 7);
    
    // Initialize all jars
    for (int i = 0; i < 8; i++) {
        jars[i]->begin();
    }
    
    // Initialize separate SPI bus for SD card (HSPI)
    pinMode(cs, OUTPUT);
    digitalWrite(cs, HIGH);
    spiSD.begin(sck, miso, mosi, cs);
    
    // Initialize SD Card on separate SPI
    if (!SD.begin(cs, spiSD, 4000000)) {
        Serial.println("SD Card Mount Failed");
        sdCardReady = false;
    } else {
        Serial.println("SD Card initialized successfully");
        sdCardReady = true;
        
        uint8_t cardType = SD.cardType();
        Serial.printf("SD Card Type: %d\n", cardType);
        Serial.printf("SD Card Size: %lluMB\n", SD.cardSize() / (1024 * 1024));
        
        listDir(SD, "/", 0);
    }
    
    // Initialize LVGL
    lv_init();
    
    // Initialize TFT Display (uses default VSPI)
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
    
    // Initialize Touchscreen
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);
    
    ts.begin();
    ts.setRotation(1);
    
    // Initialize UI
    ui_init();
    
    // Register Home Screen Jar Selection Buttons (UPDATE WITH YOUR BUTTON OBJECT NAMES!)
    // These buttons are on your home screen - one for each jar
    lv_obj_add_event_cb(objects.jar1_button, select_jar_cb, LV_EVENT_CLICKED, (void*)0);
    lv_obj_add_event_cb(objects.jar2_button, select_jar_cb, LV_EVENT_CLICKED, (void*)1);
    lv_obj_add_event_cb(objects.jar3_button, select_jar_cb, LV_EVENT_CLICKED, (void*)2);
    lv_obj_add_event_cb(objects.jar4_button, select_jar_cb, LV_EVENT_CLICKED, (void*)3);
    lv_obj_add_event_cb(objects.jar5_button, select_jar_cb, LV_EVENT_CLICKED, (void*)4);
    lv_obj_add_event_cb(objects.jar6_button, select_jar_cb, LV_EVENT_CLICKED, (void*)5);
    lv_obj_add_event_cb(objects.jar7_button, select_jar_cb, LV_EVENT_CLICKED, (void*)6);
    lv_obj_add_event_cb(objects.jar8_button, select_jar_cb, LV_EVENT_CLICKED, (void*)7);
    
    // Register Shared Info Screen Event Callbacks (same screen used for all jars!)
    lv_obj_add_event_cb(objects.info_page_rpm_increase_icon, rpm_plus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.info_page_rpm_decrease_icon, rpm_minus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.info_page_time_increase_icon, time_plus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.info_page_time_decrease_icon, time_minus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.config_motor_rotation_direction_switch, direction_switch_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(objects.jar_info_start_button, start_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.jar_info_stop, stop_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.jar_info_save, save_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.rpm_textarea, rpm_textarea_cb, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(objects.duration_textarea, duration_textarea_cb, LV_EVENT_READY, NULL);
    
    // Bind shared UI objects to all jars (they all use the same UI screen)
    for (int i = 0; i < 8; i++) {
        jars[i]->bindUI(
            objects.rpm_textarea,
            objects.duration_textarea,
            objects.jar1_state,
            objects.jar1_time_countdown,
            objects.readings_rpm,
            objects.readings_timeleft,
            objects.jar_info_save,
            objects.jar_info_start_button,
            objects.jar_info_stop,
            objects.config_motor_rotation_direction_switch
        );
    }
    
    // Create Timer for Motor Control (1 second interval)
    lv_timer_create(timer_cb, 1000, NULL);
    
    Serial.println("=== Setup Complete ===");
}

// ==================== Main Loop ====================
void loop() {
    // Handle LVGL tasks
    lv_tick_inc(5);
    lv_timer_handler();
    ui_tick();
    
    delay(5);
}
