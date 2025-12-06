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

static const uint16_t screenWidth = 320;
static const uint16_t screenHeight = 240;

#define TOUCH_CS 21
#define TIRQ_PIN 27




// Motor pins for 8 Jars
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

// SPI for SD Card
const int sck = 14;
const int miso = 12;
const int mosi = 13;
const int cs = 5;

TFT_eSPI tft(screenWidth, screenHeight);
XPT2046_Touchscreen ts(TOUCH_CS, TIRQ_PIN);
SPIClass spiSD(HSPI);

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 40];

bool sdCardReady = false;

// Batch configuration variables
bool selectedJars[8] = {false, false, false, false, false, false, false, false};
int batchRPM = 0;
int batchDuration = 0;
bool batchDirection = true;



JarController* jars[8];
JarController* currentJar = nullptr;
int currentJarIndex = -1;




// ===== CHECKBOX CALLBACKS =====
void jar1_checkbox_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        selectedJars[0] = lv_obj_has_state(objects.jar1_checkbox, LV_STATE_CHECKED);
        Serial.printf("Jar 1 checkbox: %s\n", selectedJars[0] ? "SELECTED" : "UNSELECTED");
    }
}

void jar2_checkbox_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        selectedJars[1] = lv_obj_has_state(objects.jar2_checkbox, LV_STATE_CHECKED);
        Serial.printf("Jar 2 checkbox: %s\n", selectedJars[1] ? "SELECTED" : "UNSELECTED");
    }
}

void jar3_checkbox_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        selectedJars[2] = lv_obj_has_state(objects.jar3_checkbox, LV_STATE_CHECKED);
        Serial.printf("Jar 3 checkbox: %s\n", selectedJars[2] ? "SELECTED" : "UNSELECTED");
    }
}

void jar4_checkbox_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        selectedJars[3] = lv_obj_has_state(objects.jar4_checkbox, LV_STATE_CHECKED);
        Serial.printf("Jar 4 checkbox: %s\n", selectedJars[3] ? "SELECTED" : "UNSELECTED");
    }
}

void jar5_checkbox_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        selectedJars[4] = lv_obj_has_state(objects.jar5_checkbox, LV_STATE_CHECKED);
        Serial.printf("Jar 5 checkbox: %s\n", selectedJars[4] ? "SELECTED" : "UNSELECTED");
    }
}

void jar6_checkbox_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        selectedJars[5] = lv_obj_has_state(objects.jar6_checkbox, LV_STATE_CHECKED);
        Serial.printf("Jar 6 checkbox: %s\n", selectedJars[5] ? "SELECTED" : "UNSELECTED");
    }
}

void jar7_checkbox_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        selectedJars[6] = lv_obj_has_state(objects.jar7_checkbox, LV_STATE_CHECKED);
        Serial.printf("Jar 7 checkbox: %s\n", selectedJars[6] ? "SELECTED" : "UNSELECTED");
    }
}

void jar8_checkbox_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        selectedJars[7] = lv_obj_has_state(objects.jar8_checkbox, LV_STATE_CHECKED);
        Serial.printf("Jar 8 checkbox: %s\n", selectedJars[7] ? "SELECTED" : "UNSELECTED");
    }
}







// ===== BATCH RPM INCREMENT/DECREMENT =====
void batch_rpm_plus_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        batchRPM = constrain(batchRPM + 5, 0, 100);
        char buf[8];
        sprintf(buf, "%d", batchRPM);
        lv_textarea_set_text(objects.rpm_textarea, buf);
        Serial.printf("Batch RPM: %d\n", batchRPM);
    }
}

void batch_rpm_minus_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        batchRPM = constrain(batchRPM - 5, 0, 100);
        char buf[8];
        sprintf(buf, "%d", batchRPM);
        lv_textarea_set_text(objects.rpm_textarea, buf);
        Serial.printf("Batch RPM: %d\n", batchRPM);
    }
}

// ===== BATCH DURATION INCREMENT/DECREMENT =====
void batch_duration_plus_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        batchDuration = constrain(batchDuration + 10, 0, 3600);
        char buf[8];
        sprintf(buf, "%d", batchDuration);
        lv_textarea_set_text(objects.duration_textarea, buf);
        Serial.printf("Batch Duration: %d minutes\n", batchDuration);
    }
}

void batch_duration_minus_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        batchDuration = constrain(batchDuration - 10, 0, 3600);
        char buf[8];
        sprintf(buf, "%d", batchDuration);
        lv_textarea_set_text(objects.duration_textarea, buf);
        Serial.printf("Batch Duration: %d minutes\n", batchDuration);
    }
}

// ===== BATCH TEXTAREA INPUT =====
void batch_rpm_textarea_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_READY) {
        const char *txt = lv_textarea_get_text(objects.rpm_textarea);
        batchRPM = constrain(atoi(txt), 0, 100);
        Serial.printf("Batch RPM set to: %d\n", batchRPM);
    }
}

void batch_duration_textarea_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_READY) {
        const char *txt = lv_textarea_get_text(objects.duration_textarea);
        batchDuration = constrain(atoi(txt), 0, 3600);
        Serial.printf("Batch Duration set to: %d minutes\n", batchDuration);
    }
}

// ===== BATCH DIRECTION SWITCH =====
void batch_direction_switch_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        batchDirection = lv_obj_has_state(objects.config_motor_rotation_direction_switch, LV_STATE_CHECKED);
        Serial.printf("Batch Direction: %s\n", batchDirection ? "CW" : "CCW");
    }
}

// ===== BATCH SAVE BUTTON =====
void batch_save_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        Serial.println("=== BATCH SAVE STARTED ===");



// Directly read checkbox states at save time
        selectedJars[0] = lv_obj_has_state(objects.jar1_checkbox, LV_STATE_CHECKED);
        selectedJars[1] = lv_obj_has_state(objects.jar2_checkbox, LV_STATE_CHECKED);
        selectedJars[2] = lv_obj_has_state(objects.jar3_checkbox, LV_STATE_CHECKED);
        selectedJars[3] = lv_obj_has_state(objects.jar4_checkbox, LV_STATE_CHECKED);
        selectedJars[4] = lv_obj_has_state(objects.jar5_checkbox, LV_STATE_CHECKED);
        selectedJars[5] = lv_obj_has_state(objects.jar6_checkbox, LV_STATE_CHECKED);
        selectedJars[6] = lv_obj_has_state(objects.jar7_checkbox, LV_STATE_CHECKED);
        selectedJars[7] = lv_obj_has_state(objects.jar8_checkbox, LV_STATE_CHECKED);
        
        // Debug: Print all checkbox states
        Serial.println("Checkbox states:");
        for (int i = 0; i < 8; i++) {
            Serial.printf("  Jar %d: %s\n", i + 1, selectedJars[i] ? "CHECKED" : "UNCHECKED");
        }

        
        int savedCount = 0;
        bool anySelected = false;
        
        for (int i = 0; i < 8; i++) {
            if (selectedJars[i]) {
                anySelected = true;
                
                // Apply settings to selected jar
                jars[i]->setRPMValue(batchRPM);
                jars[i]->setDurationValue(batchDuration);
                jars[i]->setDirection(batchDirection);
                
                // Save to SD card
                if (sdCardReady) {
                    if (jars[i]->saveSettings("/jar_settings.txt")) {
                        savedCount++;
                        Serial.printf("✓ Jar %d: RPM=%d, Duration=%d min, Direction=%s\n", 
                                     i + 1, batchRPM, batchDuration, batchDirection ? "CW" : "CCW");
                    }
                } else {
                    Serial.printf("✗ Jar %d: SD card not ready\n", i + 1);
                }
            }
        }
        
        if (!anySelected) {
            Serial.println("⚠ No jars selected!");
            if (objects.all_jar_configurations_save_button) {
                lv_obj_set_style_bg_color(objects.all_jar_configurations_save_button, 
                                         lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
            }
        } else {
            Serial.printf("=== BATCH SAVE COMPLETE: %d/%d jars saved ===\n", savedCount, 8);
            
            // Visual feedback - green button
            if (objects.all_jar_configurations_save_button) {
                lv_obj_set_style_bg_color(objects.all_jar_configurations_save_button, 
                                         lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
            }
        }
    }
}

// ===== BATCH START BUTTON =====
void batch_start_all_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        Serial.println("=== BATCH START INITIATED ===");
        
        int startedCount = 0;
        bool anySelected = false;
        
        for (int i = 0; i < 8; i++) {
            if (selectedJars[i]) {
                anySelected = true;
                
                // Check if jar is properly configured
                if (!jars[i]->areSettingsSaved()) {
                    Serial.printf("✗ Jar %d: Settings not saved\n", i + 1);
                    continue;
                }
                
                if (jars[i]->getRPM() <= 0 || jars[i]->getDuration() <= 0) {
                    Serial.printf("✗ Jar %d: Invalid RPM or Duration\n", i + 1);
                    continue;
                }
                
                // Start the jar
                jars[i]->startMotor();
                startedCount++;
                Serial.printf("✓ Jar %d started successfully\n", i + 1);
            }
        }
        
        if (!anySelected) {
            Serial.println("⚠ No jars selected!");
        } else {
            Serial.printf("=== BATCH START COMPLETE: %d jars running ===\n", startedCount);
        }
    }
}

// ===== BATCH STOP BUTTON =====
void batch_stop_all_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        Serial.println("=== BATCH STOP INITIATED ===");
        
        int stoppedCount = 0;
        
        for (int i = 0; i < 8; i++) {
            if (selectedJars[i] && jars[i]->isMotorRunning()) {
                jars[i]->stopMotor();
                stoppedCount++;
                Serial.printf("✓ Jar %d stopped\n", i + 1);
            }
        }
        
        Serial.printf("=== BATCH STOP COMPLETE: %d jars stopped ===\n", stoppedCount);
    }
}


void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
    Serial.printf("Listing directory: %s\n", dirname);
    File root = fs.open(dirname);
    if (!root || !root.isDirectory()) {
        Serial.println("Failed to open directory");
        return;
    }
    
    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels) listDir(fs, file.path(), levels - 1);
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

void updateUIForCurrentJar() {
    if (currentJar != nullptr) {
        currentJar->updateUIFields();
        

     currentJar->updateButtonStates(currentJar->isMotorRunning());

        // Update info page readings display
        char buf[32];
        sprintf(buf, "RPM: %d", currentJar->getRPM());
        lv_label_set_text(objects.readings_rpm, buf);
        
        // Update jar number on info page
        char jarTitle[32];
        sprintf(jarTitle, "JAR %d", currentJarIndex + 1);
        if (objects.jar_number_on_info_screen) {
            lv_label_set_text(objects.jar_number_on_info_screen, jarTitle);
        }
        
        Serial.printf("Switched to Jar %d\n", currentJarIndex + 1);
    }
}

// Update info page countdown display
void updateInfoPageDisplay() {
    if (currentJar != nullptr) {
        unsigned long remainingMillis = currentJar->getRemainingTime();
        int secs = remainingMillis / 1000;
        int min = secs / 60;
        int sec = secs % 60;
        
        char buf[32];
        sprintf(buf, "Time left: %02d:%02d", min, sec);
        lv_label_set_text(objects.readings_timeleft, buf);
    }
}

// Jar selection callback
void select_jar_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        int jarIndex = (int)(intptr_t)lv_event_get_user_data(e);
        currentJarIndex = jarIndex;
        currentJar = jars[jarIndex];


        Serial.printf("Selected Jar %d\n", jarIndex + 1);
        
        // Navigate to info screen (uncomment your screen load)
        // lv_scr_load(objects.info_screen);
        
        updateUIForCurrentJar();
    }
}

// Event Callbacks
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

// NEW: Start All Jars
void start_all_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        Serial.println("=== START ALL JARS ===");
        
        for (int i = 0; i < 8; i++) {
            if (jars[i]->areSettingsSaved() && jars[i]->getRPM() > 0 && jars[i]->getDuration() > 0) {
                jars[i]->startMotor();
                Serial.printf("Jar %d started\n", i + 1);
            } else {
                Serial.printf("Jar %d skipped (settings not saved or invalid)\n", i + 1);
            }
        }
    }
}

// NEW: Stop All Jars
void stop_all_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        Serial.println("=== STOP ALL JARS ===");
        
        for (int i = 0; i < 8; i++) {
            if (jars[i]->isMotorRunning()) {
                jars[i]->stopMotor();
                Serial.printf("Jar %d stopped\n", i + 1);
            }
        }
    }
}

void timer_cb(lv_timer_t *timer) {
    // Update all jars (updates home page countdown for each jar independently)
    for (int i = 0; i < 8; i++) {
        jars[i]->onTimerTick();
    }
    
    // Update info page display if viewing a jar
    updateInfoPageDisplay();
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
    }
    else {
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
    
    for (int i = 0; i < 8; i++) {
        jars[i]->begin();
    }
    
    // Initialize SD Card
    pinMode(cs, OUTPUT);
    digitalWrite(cs, HIGH);
    spiSD.begin(sck, miso, mosi, cs);
    
    if (!SD.begin(cs, spiSD, 4000000)) {
        Serial.println("SD Card Mount Failed");
        sdCardReady = false;
    } else {
        Serial.println("SD Card initialized successfully");
        sdCardReady = true;
        listDir(SD, "/", 0);
    }
    
    // Initialize LVGL
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
    
    // Register jar selection buttons (UPDATE WITH YOUR ACTUAL BUTTON NAMES!)
    lv_obj_add_event_cb(objects.jar1_button, select_jar_cb, LV_EVENT_CLICKED, (void*)0);
    lv_obj_add_event_cb(objects.jar2_button, select_jar_cb, LV_EVENT_CLICKED, (void*)1);
    lv_obj_add_event_cb(objects.jar3_button, select_jar_cb, LV_EVENT_CLICKED, (void*)2);
    lv_obj_add_event_cb(objects.jar4_button, select_jar_cb, LV_EVENT_CLICKED, (void*)3);
    lv_obj_add_event_cb(objects.jar5_button, select_jar_cb, LV_EVENT_CLICKED, (void*)4);
    lv_obj_add_event_cb(objects.jar6_button, select_jar_cb, LV_EVENT_CLICKED, (void*)5);
    lv_obj_add_event_cb(objects.jar7_button, select_jar_cb, LV_EVENT_CLICKED, (void*)6);
    lv_obj_add_event_cb(objects.jar8_button, select_jar_cb, LV_EVENT_CLICKED, (void*)7);






// RPM text input and increment/decrement (reusing existing callbacks)
    lv_obj_add_event_cb(objects.rpm_textarea, batch_rpm_textarea_cb, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(objects.rpm_increase_icon, batch_rpm_plus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.rpm_decrease_icon, batch_rpm_minus_cb, LV_EVENT_CLICKED, NULL);
    
    // Duration text input and increment/decrement
    lv_obj_add_event_cb(objects.duration_textarea, batch_duration_textarea_cb, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(objects.time_increase_icon, batch_duration_plus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.time_decrease_icon, batch_duration_minus_cb, LV_EVENT_CLICKED, NULL);
    
    // Direction switch
    lv_obj_add_event_cb(objects.config_motor_rotation_direction_switch, batch_direction_switch_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Save and Start/Stop buttons
    lv_obj_add_event_cb(objects.all_jar_configurations_save_button, batch_save_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.start_all_button, batch_start_all_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.stop_all_button, batch_stop_all_cb, LV_EVENT_CLICKED, NULL);
    
    Serial.println("=== Batch Configuration Callbacks Registered ===");



    
    // Register START ALL and STOP ALL buttons
    lv_obj_add_event_cb(objects.start_all_button_1, start_all_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.stop_all_button_1, stop_all_cb, LV_EVENT_CLICKED, NULL);
    
    // Register Info Page event callbacks
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
    
    // Bind Info Page UI (shared across all jars - INPUT CONTROLS ONLY!)
    // NO countdown/state labels bound here to prevent conflicts with home page
    for (int i = 0; i < 8; i++) {
        jars[i]->bindInfoPageUI(
            objects.rpm_textarea,
            objects.duration_textarea,
            objects.jar_info_save,
            objects.jar_info_start_button,
            objects.jar_info_stop,
            objects.config_motor_rotation_direction_switch
        );
    }
    
    // Bind Home Page UI (individual for each jar) - USING YOUR EXISTING UI OBJECTS!
    jars[0]->bindHomePageUI(objects.jar1_time_countdown, objects.jar1_state);
    jars[1]->bindHomePageUI(objects.jar2_time_countdown, objects.jar2_state);
    jars[2]->bindHomePageUI(objects.jar3_time_countdown, objects.jar3_state);
    jars[3]->bindHomePageUI(objects.jar4_time_countdown, objects.jar4_state);
    jars[4]->bindHomePageUI(objects.jar5_time_countdown, objects.jar5_state);
    jars[5]->bindHomePageUI(objects.jar6_time_countdown, objects.jar6_state);
    jars[6]->bindHomePageUI(objects.jar7_time_countdown, objects.jar7_state);
    jars[7]->bindHomePageUI(objects.jar8_time_countdown, objects.jar8_state);
    
    // Create Timer
    lv_timer_create(timer_cb, 1000, NULL);
    
    Serial.println("=== Setup Complete ===");
}

void loop() {
    lv_tick_inc(5);
    lv_timer_handler();
    ui_tick();
    


    delay(5);
}