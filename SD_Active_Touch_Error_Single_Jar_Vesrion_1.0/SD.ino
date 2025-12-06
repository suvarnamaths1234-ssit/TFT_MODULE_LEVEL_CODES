
#include <lvgl.h>
#include <TFT_eSPI.h>
#include "ui.h"
#include "vars.h"
#include "screens.h"
#include <XPT2046_Touchscreen.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

extern objects_t objects;

// Display Configuration
static const uint16_t screenWidth = 320;
static const uint16_t screenHeight = 240;

// Pin Definitions
#define TOUCH_CS 21
#define TIRQ_PIN 27
#define motorEnablePin 25
#define motorDirectionPin 26

// SPI Pin Configuration
const int sck = 14;
const int miso = 12;
const int mosi = 13;
const int cs = 5;

// Motor PWM Configuration
const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;
const int duty_percentage = 80;

// Hardware Objects
TFT_eSPI tft(screenWidth, screenHeight);
XPT2046_Touchscreen ts(TOUCH_CS, TIRQ_PIN);

// LVGL Display Buffer
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 40];

// Global State Variables
bool motorOn = false;
int setRPM = 0;
int setDuration = 0;
bool directionCW = true;
bool jar1_saved = false;
bool sdCardReady = false;

unsigned long motorStartMillis = 0;
unsigned long elapsedMillis = 0;

using namespace fs;

// ==================== SD Card Helper Functions ====================

bool acquireSDCard() {
    // Force end any TFT SPI transactions
    tft.endWrite();
    
    // Reset SPI bus
    SPI.end();
    delay(50);
    
    // Reinitialize SPI for SD card
    SPI.begin(sck, miso, mosi, cs);
    SPI.setFrequency(4000000);  // 4MHz for SD card
    
    // Attempt to reinitialize SD card (3 attempts)
    for (int attempt = 0; attempt < 3; attempt++) {
        if (SD.begin(cs, SPI, 4000000)) {
            Serial.println("SD card acquired successfully");
            return true;
        }
        delay(100);
    }
    
    Serial.println("SD card acquisition failed after 3 attempts");
    return false;
}

void checkSDStatus() {
    Serial.println("=== SD Card Status ===");
    Serial.printf("SD Ready Flag: %s\n", sdCardReady ? "true" : "false");
    
    if (!acquireSDCard()) {
        Serial.println("SD card not accessible");
        return;
    }
    
    uint8_t cardType = SD.cardType();
    Serial.printf("Card Type: %d\n", cardType);
    
    if (cardType != CARD_NONE) {
        Serial.printf("Card Size: %lluMB\n", SD.cardSize() / (1024 * 1024));
        Serial.printf("Total Space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
        Serial.printf("Used Space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
    }
    Serial.println("===================");
}

// ==================== SD Card File Operations ====================

void writeFile(fs::FS &fs, const char *path, const char *message) {
    if (!acquireSDCard()) {
        Serial.println("Cannot acquire SD card for writing");
        return;
    }
    
    Serial.printf("Writing file: %s\n", path);
    
    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        Serial.printf("SD Type: %d\n", SD.cardType());
        return;
    }
    
    if (file.print(message)) {
        Serial.println("File written successfully");
    } else {
        Serial.println("Write failed");
    }
    
    file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
    if (!acquireSDCard()) {
        Serial.println("Cannot acquire SD card for appending");
        return;
    }
    
    Serial.printf("Appending to file: %s\n", path);
    
    File file = fs.open(path, FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open file for appending");
        Serial.printf("SD Type: %d\n", SD.cardType());
        return;
    }
    
    if (file.print(message)) {
        Serial.println("Message appended successfully");
    } else {
        Serial.println("Append failed");
    }
    
    file.close();
}

void readFile(fs::FS &fs, const char *path) {
    if (!acquireSDCard()) {
        Serial.println("Cannot acquire SD card for reading");
        return;
    }
    
    Serial.printf("Reading file: %s\n", path);
    
    File file = fs.open(path);
    if (!file) {
        Serial.println("Failed to open file for reading");
        return;
    }
    
    Serial.println("File content:");
    while (file.available()) {
        Serial.write(file.read());
    }
    Serial.println();
    
    file.close();
}

void deleteFile(fs::FS &fs, const char *path) {
    if (!acquireSDCard()) {
        Serial.println("Cannot acquire SD card for deleting");
        return;
    }
    
    Serial.printf("Deleting file: %s\n", path);
    if (fs.remove(path)) {
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
    if (!acquireSDCard()) {
        Serial.println("Cannot acquire SD card for listing");
        return;
    }
    
    Serial.printf("Listing directory: %s\n", dirname);
    
    File root = fs.open(dirname);
    if (!root) {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        Serial.println("Not a directory");
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
}

void createDir(fs::FS &fs, const char *path) {
    if (!acquireSDCard()) {
        Serial.println("Cannot acquire SD card for creating directory");
        return;
    }
    
    Serial.printf("Creating Dir: %s\n", path);
    if (fs.mkdir(path)) {
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

void removeDir(fs::FS &fs, const char *path) {
    if (!acquireSDCard()) {
        Serial.println("Cannot acquire SD card for removing directory");
        return;
    }
    
    Serial.printf("Removing Dir: %s\n", path);
    if (fs.rmdir(path)) {
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}

void renameFile(fs::FS &fs, const char *path1, const char *path2) {
    if (!acquireSDCard()) {
        Serial.println("Cannot acquire SD card for renaming");
        return;
    }
    
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
}

// ==================== UI Helper Functions ====================

void update_jar1_state_label(const char *txt, lv_palette_t color) {
    if (objects.jar1_state) {
        lv_label_set_text(objects.jar1_state, txt);
        lv_obj_set_style_text_color(objects.jar1_state, lv_palette_main(color), LV_PART_MAIN);
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

void updateUICountdown(unsigned long remainingMillis) {
    int secs = remainingMillis / 1000;
    int min = secs / 60;
    int sec = secs % 60;
    char buf[16];
    sprintf(buf, "%02d:%02d", min, sec);
    lv_label_set_text(objects.jar1_time_countdown, buf);
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
    char rpmBuf[8], durBuf[8];
    sprintf(rpmBuf, "%d", setRPM);
    sprintf(durBuf, "%d", setDuration);
    lv_textarea_set_text(objects.rpm_textarea, rpmBuf);
    lv_textarea_set_text(objects.duration_textarea, durBuf);
    updateRpmReadings();
}

// ==================== Motor Control Functions ====================

void startMotor() {
    motorOn = true;
    motorStartMillis = millis();
    elapsedMillis = 0;
    
    int dutyCycle = map(setRPM, 0, 100, 0, 255);
    ledcWrite(pwmChannel, dutyCycle);
    digitalWrite(motorDirectionPin, directionCW ? HIGH : LOW);
    
    update_jar1_state_label("ON", LV_PALETTE_GREEN);
    Serial.println("Motor started");
}

void stopMotor() {
    Serial.println("Motor stopped");
    motorOn = false;
    elapsedMillis = 0;
    motorStartMillis = 0;
    ledcWrite(pwmChannel, 0);
    
    update_jar1_state_label("OFF", LV_PALETTE_RED);
}

// ==================== Event Callbacks ====================

void rpm_textarea_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_READY) {
        const char *txt = lv_textarea_get_text(objects.rpm_textarea);
        int val = atoi(txt);
        if (val >= 0 && val <= 100) {
            setRPM = val;
        }
        jar1_saved = false;
        lv_obj_set_style_bg_color(objects.jar_info_save, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
    }
}

void duration_textarea_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_READY) {
        const char *txt = lv_textarea_get_text(objects.duration_textarea);
        int val = atoi(txt);
        if (val >= 0 && val <= 3600) {
            setDuration = val;
        }
        jar1_saved = false;
        lv_obj_set_style_bg_color(objects.jar_info_save, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
    }
}

void rpm_plus_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        setRPM += 5;
        if (setRPM > 100) setRPM = 100;
        
        jar1_saved = false;
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
        
        jar1_saved = false;
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
        
        jar1_saved = false;
        lv_obj_set_style_bg_color(objects.jar_info_save, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
        updateUIFields();
    }
}

void time_minus_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        setDuration -= 10;
        if (setDuration < 0) setDuration = 0;
        
        jar1_saved = false;
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

void jar1_save_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        set_var_rpm(setRPM);
        set_var_time(setDuration);
        
        char data[64];
        sprintf(data, "RPM: %d, Duration: %d\n", get_var_rpm(), get_var_time());
        
        if (sdCardReady) {
            Serial.println("Attempting to save to SD card...");
            checkSDStatus();  // Debug info
            
            // Write initial content
            writeFile(SD, "/settings_log.txt", "=== Motor Settings Log ===\n");
            
            // Append actual data
            appendFile(SD, "/settings_log.txt", data);
            
            Serial.print("Saved to SD: ");
            Serial.println(data);
        } else {
            Serial.println("SD card not ready");
        }
        
        jar1_saved = true;
        Serial.printf("Settings saved - RPM: %d, Duration: %d\n", setRPM, setDuration);
        
        // Update UI to show saved state
        lv_obj_set_style_bg_color(objects.jar_info_save, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(objects.jar_info_save, LV_OPA_COVER, LV_PART_MAIN);
    }
}

void jar1_start_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        if (!jar1_saved) {
            Serial.println("Please save settings before starting motor");
            return;
        }
        
        if (setRPM > 0 && setDuration > 0) {
            startMotor();
            updateUIFields();
            
            // Update button UI for running state
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
        
        // Update button UI for stopped state
        update_button_label(objects.jar_info_stop, "Completed");
        update_button_label(objects.jar_info_start_button, "Start");
        lv_obj_set_style_bg_color(objects.jar_info_stop, lv_palette_main(LV_PALETTE_YELLOW), LV_PART_MAIN);
        lv_obj_set_style_bg_color(objects.jar_info_start_button, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
        
        Serial.println("Motor stopped - UI updated");
    }
}

// ==================== Timer Callback ====================

void timer_cb(lv_timer_t *timer) {
    unsigned long remainingMillis = 0;
    
    if (motorOn && setDuration > 0) {
        elapsedMillis = millis() - motorStartMillis;
        
        if (elapsedMillis >= (unsigned long)(setDuration) * 1000UL) {
            // Timer expired - stop motor
            stopMotor();
            
            // Update button UI for completed state
            update_button_label(objects.jar_info_start_button, "Start");
            update_button_label(objects.jar_info_stop, "Completed");
            lv_obj_set_style_bg_color(objects.jar_info_start_button, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
            lv_obj_set_style_bg_color(objects.jar_info_stop, lv_palette_main(LV_PALETTE_YELLOW), LV_PART_MAIN);
            
            remainingMillis = 0;
            Serial.println("Countdown expired — motor stopped");
        } else {
            remainingMillis = (setDuration * 1000UL) - elapsedMillis;
        }
    } else {
        // Motor off - ensure buttons show idle state
        update_button_label(objects.jar_info_start_button, "Start");
        update_button_label(objects.jar_info_stop, "Completed");
        lv_obj_set_style_bg_color(objects.jar_info_start_button, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
        lv_obj_set_style_bg_color(objects.jar_info_stop, lv_palette_main(LV_PALETTE_YELLOW), LV_PART_MAIN);
    }
    
    updateUICountdown(remainingMillis);
    updateTimeLeftReadings(remainingMillis);
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
    
    Serial.println("=== Motor Controller Starting ===");
    
    // Initialize SD Card FIRST (before TFT)
    pinMode(cs, OUTPUT);
    digitalWrite(cs, HIGH);
    SPI.begin(sck, miso, mosi, cs);
    delay(100);
    
    if (!SD.begin(cs, SPI, 4000000)) {
        Serial.println("SD Card Mount Failed");
        sdCardReady = false;
    } else {
        Serial.println("SD Card initialized successfully");
        sdCardReady = true;
        
        // Print SD card info
        uint8_t cardType = SD.cardType();
        Serial.printf("SD Card Type: %d\n", cardType);
        Serial.printf("SD Card Size: %lluMB\n", SD.cardSize() / (1024 * 1024));
    }
    
    // Initialize Motor Pins
    pinMode(motorDirectionPin, OUTPUT);
    digitalWrite(motorDirectionPin, LOW);
    ledcAttach(motorEnablePin, freq, resolution);
    
    // Initialize LVGL
    lv_init();
    
    // Initialize TFT Display
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
    updateUIFields();
    
    // Register Event Callbacks
    lv_obj_add_event_cb(objects.info_page_rpm_increase_icon, rpm_plus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.info_page_rpm_decrease_icon, rpm_minus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.info_page_time_increase_icon, time_plus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.info_page_time_decrease_icon, time_minus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.config_motor_rotation_direction_switch, direction_switch_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(objects.jar_info_start_button, jar1_start_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.jar_info_stop, jar1_stop_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.jar_info_save, jar1_save_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.rpm_textarea, rpm_textarea_cb, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(objects.duration_textarea, duration_textarea_cb, LV_EVENT_READY, NULL);
    
    // Create Timer for Motor Control
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