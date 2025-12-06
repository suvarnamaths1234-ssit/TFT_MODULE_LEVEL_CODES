#include "JarController.h"
#include "FS.h"
#include "SD.h"

// PWM Configuration (shared across all jars)
const int PWM_FREQ = 30000;
const int PWM_RESOLUTION = 8;



// Constructor
JarController::JarController(int id, int enablePin, int directionPin, int channel)
    : jarID(id), motorEnablePin(enablePin), motorDirectionPin(directionPin), 
      pwmChannel(channel), setRPM(0), setDuration(0), directionCW(true),
      motorOn(false), settingsSaved(false), motorStartMillis(0), elapsedMillis(0),
      ui_rpm_textarea(nullptr), ui_duration_textarea(nullptr),
      ui_save_button(nullptr), ui_start_button(nullptr), ui_stop_button(nullptr),
      ui_direction_switch(nullptr), ui_home_countdown_label(nullptr), 
      ui_home_state_label(nullptr), ui_rpm_reading_label(nullptr), 
      ui_timeleft_label(nullptr) {
}

// Initialize motor pins and PWM
void JarController::begin() {
    pinMode(motorDirectionPin, OUTPUT);
    digitalWrite(motorDirectionPin, LOW);
    ledcAttach(motorEnablePin, PWM_FREQ, PWM_RESOLUTION);
    ledcWrite(pwmChannel, 0);  // Start with motor off
    
    Serial.printf("Jar %d initialized - Enable Pin: %d, Direction Pin: %d, PWM Channel: %d\n",
                  jarID, motorEnablePin, motorDirectionPin, pwmChannel);
}

// Start the motor
void JarController::startMotor() {
    if (setRPM == 0 || setDuration == 0) {
        Serial.printf("Jar %d: Cannot start - RPM or Duration is zero\n", jarID);
        return;
    }
    
    if (!settingsSaved) {
        Serial.printf("Jar %d: Cannot start - Settings not saved\n", jarID);
        if (ui_save_button) {
            lv_obj_set_style_bg_color(ui_save_button, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
        }
        return;
    }
    
    motorOn = true;
    motorStartMillis = millis();
    elapsedMillis = 0;
    
    int dutyCycle = map(setRPM, 0, 100, 0, 255);
    ledcWrite(pwmChannel, dutyCycle);
    digitalWrite(motorDirectionPin, directionCW ? HIGH : LOW);
    
    // Update button states on info page
    updateButtonStates(true);
    
    // Update home page UI for this jar ONLY
    updateHomePageUI();
    
    Serial.printf("Jar %d: Motor started - RPM: %d, Duration: %d sec\n", 
                  jarID, setRPM, setDuration);
}

// Stop the motor
void JarController::stopMotor() {
    motorOn = false;
    elapsedMillis = 0;
    motorStartMillis = 0;
    ledcWrite(pwmChannel, 0);
    
    // Update button states on info page
    updateButtonStates(false);
    
    // Update home page UI for this jar ONLY
    updateHomePageUI();
    
    Serial.printf("Jar %d: Motor stopped\n", jarID);
}

// Update motor state (called in main loop)
void JarController::updateMotor() {
    if (motorOn && setDuration > 0) {
        elapsedMillis = millis() - motorStartMillis;
        
        if (elapsedMillis >= (unsigned long)(setDuration) * 1000UL) {
            stopMotor();
            Serial.printf("Jar %d: Timer expired - Motor stopped\n", jarID);
        }
    }
}

// Set RPM value
void JarController::setRPMValue(int rpm) {
    setRPM = constrain(rpm, 0, 100);
    markUnsaved();
    
    if (motorOn) {
        int dutyCycle = map(setRPM, 0, 100, 0, 255);
        ledcWrite(pwmChannel, dutyCycle);
    }
}

// Set duration value
void JarController::setDurationValue(int duration) {
    setDuration = constrain(duration, 0, 3600);
    markUnsaved();
}

// Set motor direction
void JarController::setDirection(bool clockwise) {
    directionCW = clockwise;
    if (motorOn) {
        digitalWrite(motorDirectionPin, directionCW ? HIGH : LOW);
    }
}

// Increment RPM
void JarController::incrementRPM(int step) {
    setRPMValue(setRPM + step);
    updateUIFields();
}

// Decrement RPM
void JarController::decrementRPM(int step) {
    setRPMValue(setRPM - step);
    if (motorOn && setRPM == 0) {
        stopMotor();
    }
    updateUIFields();
}

// Increment Duration
void JarController::incrementDuration(int step) {
    setDurationValue(setDuration + step);
    updateUIFields();
}

// Decrement Duration
void JarController::decrementDuration(int step) {
    setDurationValue(setDuration - step);
    updateUIFields();
}

// Save settings to SD card
bool JarController::saveSettings(const char *filepath) {
    char data[128];
    sprintf(data, "Jar%d,RPM:%d,Duration:%d,Direction:%s\n", 
            jarID, setRPM, setDuration, directionCW ? "CW" : "CCW");
    
    File file = SD.open(filepath, FILE_APPEND);
    if (!file) {
        Serial.printf("Jar %d: Failed to open file for saving\n", jarID);
        return false;
    }
    
    size_t written = file.print(data);
    file.close();
    
    if (written > 0) {
        settingsSaved = true;
        Serial.printf("Jar %d: Settings saved - %s", jarID, data);
        
        // Update save button color to blue
        if (ui_save_button) {
            lv_obj_set_style_bg_color(ui_save_button, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);
        }
        return true;
    }
    
    return false;
}




void readFile(fs::FS &fs, const char *path) {
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
















// Mark settings as unsaved
void JarController::markUnsaved() {
    settingsSaved = false;
    if (ui_save_button) {
        lv_obj_set_style_bg_color(ui_save_button, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
    }
}

// Bind Info Page UI (shared screen, inputs only)
void JarController::bindInfoPageUI(lv_obj_t *rpm_ta, lv_obj_t *dur_ta,
                                    lv_obj_t *save_btn, lv_obj_t *start_btn, lv_obj_t *stop_btn,
                                    lv_obj_t *dir_switch) {
    ui_rpm_textarea = rpm_ta;
    ui_duration_textarea = dur_ta;
    ui_save_button = save_btn;
    ui_start_button = start_btn;
    ui_stop_button = stop_btn;
    ui_direction_switch = dir_switch;
}

// Bind Home Page UI (individual for each jar)
void JarController::bindHomePageUI(lv_obj_t *home_countdown, lv_obj_t *home_state) {
    ui_home_countdown_label = home_countdown;
    ui_home_state_label = home_state;
}

// Get remaining time in milliseconds
unsigned long JarController::getRemainingTime() const {
    if (!motorOn || setDuration == 0) return 0;
    
    unsigned long elapsed = millis() - motorStartMillis;
    unsigned long totalMillis = (unsigned long)setDuration * 1000UL;
    
    if (elapsed >= totalMillis) return 0;
    return totalMillis - elapsed;
}

// Update UI text fields (info page inputs only)
void JarController::updateUIFields() {
    if (ui_rpm_textarea) {
        char buf[8];
        sprintf(buf, "%d", setRPM);
        lv_textarea_set_text(ui_rpm_textarea, buf);
    }
    
    if (ui_duration_textarea) {
        char buf[8];
        sprintf(buf, "%d", setDuration);
        lv_textarea_set_text(ui_duration_textarea, buf);
    }
    
    if (ui_direction_switch) {
        if (directionCW) {
            lv_obj_add_state(ui_direction_switch, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(ui_direction_switch, LV_STATE_CHECKED);
        }
    }
}

// Update home page UI for THIS specific jar ONLY
void JarController::updateHomePageUI() {
    unsigned long remainingMillis = getRemainingTime();
    
    int secs = remainingMillis / 1000;
    int min = secs / 60;
    int sec = secs % 60;
    
    // Update home page countdown for THIS jar ONLY
    if (ui_home_countdown_label) {
        char buf[16];
        sprintf(buf, "%02d:%02d", min, sec);
        lv_label_set_text(ui_home_countdown_label, buf);
    }
    
    // Update home page state for THIS jar ONLY
    if (ui_home_state_label) {
        if (motorOn) {
            lv_label_set_text(ui_home_state_label, "ON");
            lv_obj_set_style_text_color(ui_home_state_label, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
        } else {
            lv_label_set_text(ui_home_state_label, "OFF");
            lv_obj_set_style_text_color(ui_home_state_label, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
        }
    }
}



// Update button states based on motor running status
void JarController::updateButtonStates(bool running) {

    if (running) {
        if (ui_start_button) {
            lv_obj_t *label = lv_obj_get_child(ui_start_button, 0);
            if (label) lv_label_set_text(label, "Running");
            lv_obj_set_style_bg_color(ui_start_button, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_MAIN);
        }
        if (ui_stop_button) {
            lv_obj_t *label = lv_obj_get_child(ui_stop_button, 0);
            if (label) lv_label_set_text(label, "Stop");
            lv_obj_set_style_bg_color(ui_stop_button, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
        }
    } else {
        if (ui_start_button) {
            lv_obj_t *label = lv_obj_get_child(ui_start_button, 0);
            if (label) lv_label_set_text(label, "Start");
            lv_obj_set_style_bg_color(ui_start_button, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
        }
        if (ui_stop_button) {
            lv_obj_t *label = lv_obj_get_child(ui_stop_button, 0);
            if (label) lv_label_set_text(label, "Completed");
            lv_obj_set_style_bg_color(ui_stop_button, lv_palette_main(LV_PALETTE_YELLOW), LV_PART_MAIN);
        }
    }
}

// Timer tick callback (called every second)
void JarController::onTimerTick() {
    updateMotor();
    updateHomePageUI();  // Always update home page for THIS jar only
}



