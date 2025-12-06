#include "JarController.h"
#include "FS.h"
#include "SD.h"

const int PWM_FREQ = 30000;
const int PWM_RESOLUTION = 8;

JarController::JarController(int id, int enablePin, int directionPin, int channel)
    : jarID(id), motorEnablePin(enablePin), motorDirectionPin(directionPin), 
      pwmChannel(channel), setRPM(0), setDuration(0), directionCW(true),
      motorOn(false), settingsSaved(false), motorStartMillis(0), elapsedMillis(0),
      lastMotorCheckMillis(0), consecutiveFailures(0), errorManager(nullptr),
      ui_rpm_textarea(nullptr), ui_duration_textarea(nullptr),
      ui_save_button(nullptr), ui_start_button(nullptr), ui_stop_button(nullptr),
      ui_direction_switch(nullptr), ui_home_countdown_label(nullptr), 
      ui_home_state_label(nullptr), ui_rpm_reading_label(nullptr), 
      ui_timeleft_label(nullptr) {
}

void JarController::begin() {
    pinMode(motorDirectionPin, OUTPUT);
    digitalWrite(motorDirectionPin, LOW);
    
    ledcAttach(motorEnablePin, PWM_FREQ, PWM_RESOLUTION);
    ledcWrite(pwmChannel, 0);
    
    delay(10);
    if (ledcRead(pwmChannel) != 0) {
        if (errorManager) {
            errorManager->logError(ERR_PWM_INIT_FAILED, jarID, "PWM channel failed to initialize");
        }
        Serial.printf("Jar %d: PWM initialization error\n", jarID);
    } else {
        Serial.printf("Jar %d initialized successfully\n", jarID);
    }
}

bool JarController::validateSettings() {
    if (!ErrorManager::isValidRPM(setRPM)) {
        if (errorManager) {
            char msg[64];
            snprintf(msg, sizeof(msg), "Invalid RPM: %d (Range: 0-100)", setRPM);
            errorManager->logError(ERR_INVALID_RPM, jarID, msg);
        }
        return false;
    }
    
    if (!ErrorManager::isValidDuration(setDuration)) {
        if (errorManager) {
            char msg[64];
            snprintf(msg, sizeof(msg), "Invalid Duration: %d (Range: 0-3600)", setDuration);
            errorManager->logError(ERR_INVALID_DURATION, jarID, msg);
        }
        return false;
    }
    
    return true;
}

bool JarController::checkMotorHealth() {
    if (motorOn && elapsedMillis > (unsigned long)setDuration * 1000UL + 5000) {
        if (errorManager) {
            errorManager->logError(ERR_JAR_TIMEOUT, jarID, "Motor operation timeout");
        }
        return false;
    }
    
    return true;
}

bool JarController::startMotor() {
    if (!validateSettings()) {
        if (errorManager) {
            errorManager->logError(ERR_INVALID_RPM, jarID, "Settings validation failed");
        }
        return false;
    }
    
    if (setRPM == 0 || setDuration == 0) {
        if (errorManager) {
            char msg[64];
            snprintf(msg, sizeof(msg), "Invalid params: RPM=%d, Duration=%d", setRPM, setDuration);
            errorManager->logError(ERR_INVALID_RPM, jarID, msg);
        }
        Serial.printf("Jar %d: Cannot start - RPM or Duration is zero\n", jarID);
        return false;
    }
    
    if (!settingsSaved) {
        if (errorManager) {
            errorManager->logError(ERR_SETTINGS_NOT_SAVED, jarID, "Settings must be saved before starting");
        }
        Serial.printf("Jar %d: Cannot start - Settings not saved\n", jarID);
        if (ui_save_button) {
            lv_obj_set_style_bg_color(ui_save_button, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
        }
        return false;
    }
    
    motorOn = true;
    motorStartMillis = millis();
    elapsedMillis = 0;
    lastMotorCheckMillis = millis();
    consecutiveFailures = 0;
    
    int dutyCycle = map(setRPM, 0, 100, 0, 255);
    ledcWrite(pwmChannel, dutyCycle);
    digitalWrite(motorDirectionPin, directionCW ? HIGH : LOW);
    
    updateButtonStates(true);
    updateHomePageUI();
    
    Serial.printf("Jar %d: Motor started - RPM: %d, Duration: %d sec\n", 
                  jarID, setRPM, setDuration);
    return true;
}

void JarController::stopMotor(bool isError) {
    motorOn = false;
    elapsedMillis = 0;
    motorStartMillis = 0;
    ledcWrite(pwmChannel, 0);
    
    updateButtonStates(false);
    updateHomePageUI();
    
    if (isError) {
        if (errorManager) {
            errorManager->logError(ERR_MOTOR_STALLED, jarID, "Motor stopped due to error");
        }
        Serial.printf("Jar %d: Motor stopped due to error\n", jarID);
    } else {
        Serial.printf("Jar %d: Motor stopped normally\n", jarID);
    }
}

void JarController::updateMotor() {
    if (motorOn && setDuration > 0) {
        elapsedMillis = millis() - motorStartMillis;
        
        if (millis() - lastMotorCheckMillis >= 5000) {
            if (!checkMotorHealth()) {
                consecutiveFailures++;
                if (consecutiveFailures >= 3) {
                    stopMotor(true);
                    return;
                }
            } else {
                consecutiveFailures = 0;
            }
            lastMotorCheckMillis = millis();
        }
        
        if (elapsedMillis >= (unsigned long)(setDuration) * 1000UL) {
            stopMotor(false);
            Serial.printf("Jar %d: Timer expired - Motor stopped\n", jarID);
        }
    }
}

void JarController::setRPMValue(int rpm) {
    if (!ErrorManager::isValidRPM(rpm)) {
        if (errorManager) {
            char msg[64];
            snprintf(msg, sizeof(msg), "Attempted invalid RPM: %d", rpm);
            errorManager->logError(ERR_INVALID_RPM, jarID, msg);
        }
        return;
    }
    
    setRPM = constrain(rpm, 0, 100);
    markUnsaved();
    
    if (motorOn) {
        int dutyCycle = map(setRPM, 0, 100, 0, 255);
        ledcWrite(pwmChannel, dutyCycle);
    }
}

void JarController::setDurationValue(int duration) {
    if (!ErrorManager::isValidDuration(duration)) {
        if (errorManager) {
            char msg[64];
            snprintf(msg, sizeof(msg), "Attempted invalid duration: %d", duration);
            errorManager->logError(ERR_INVALID_DURATION, jarID, msg);
        }
        return;
    }
    
    setDuration = constrain(duration, 0, 3600);
    markUnsaved();
}

void JarController::setDirection(bool clockwise) {
    directionCW = clockwise;
    if (motorOn) {
        digitalWrite(motorDirectionPin, directionCW ? HIGH : LOW);
    }
}

void JarController::incrementRPM(int step) {
    setRPMValue(setRPM + step);
    updateUIFields();
}

void JarController::decrementRPM(int step) {
    setRPMValue(setRPM - step);
    if (motorOn && setRPM == 0) {
        stopMotor();
    }
    updateUIFields();
}

void JarController::incrementDuration(int step) {
    setDurationValue(setDuration + step);
    updateUIFields();
}

void JarController::decrementDuration(int step) {
    setDurationValue(setDuration - step);
    updateUIFields();
}

bool JarController::saveSettings(const char *filepath) {
    if (!validateSettings()) {
        Serial.printf("Jar %d: Settings validation failed\n", jarID);
        return false;
    }
    
    char data[128];
    sprintf(data, "Jar%d,RPM:%d,Duration:%d,Direction:%s\n", 
            jarID, setRPM, setDuration, directionCW ? "CW" : "CCW");
    
    File file = SD.open(filepath, FILE_APPEND);
    if (!file) {
        if (errorManager) {
            char msg[64];
            snprintf(msg, sizeof(msg), "Failed to open %s", filepath);
            errorManager->logError(ERR_SD_WRITE_FAILED, jarID, msg);
        }
        Serial.printf("Jar %d: Failed to open file for saving\n", jarID);
        return false;
    }
    
    size_t written = file.print(data);
    file.close();
    
    if (written > 0) {
        settingsSaved = true;
        Serial.printf("Jar %d: Settings saved - %s", jarID, data);
        
        if (ui_save_button) {
            lv_obj_set_style_bg_color(ui_save_button, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);
        }
        return true;
    } else {
        if (errorManager) {
            errorManager->logError(ERR_SD_WRITE_FAILED, jarID, "Zero bytes written to file");
        }
        return false;
    }
}

void JarController::markUnsaved() {
    settingsSaved = false;
    if (ui_save_button) {
        lv_obj_set_style_bg_color(ui_save_button, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
    }
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

void JarController::bindHomePageUI(lv_obj_t *home_countdown, lv_obj_t *home_state) {
    ui_home_countdown_label = home_countdown;
    ui_home_state_label = home_state;
}

unsigned long JarController::getRemainingTime() const {
    if (!motorOn || setDuration == 0) return 0;
    
    unsigned long elapsed = millis() - motorStartMillis;
    unsigned long totalMillis = (unsigned long)setDuration * 1000UL;
    
    if (elapsed >= totalMillis) return 0;
    return totalMillis - elapsed;
}

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

void JarController::updateHomePageUI() {
    unsigned long remainingMillis = getRemainingTime();
    
    int secs = remainingMillis / 1000;
    int min = secs / 60;
    int sec = secs % 60;
    
    if (ui_home_countdown_label) {
        char buf[16];
        sprintf(buf, "%02d:%02d", min, sec);
        lv_label_set_text(ui_home_countdown_label, buf);
    }
    
    if (ui_home_state_label) {
        if (motorOn) {
            lv_label_set_text(ui_home_state_label, hasError() ? "ERROR" : "ON");
            lv_obj_set_style_text_color(ui_home_state_label, 
                hasError() ? lv_palette_main(LV_PALETTE_RED) : lv_palette_main(LV_PALETTE_GREEN), 
                LV_PART_MAIN);
        } else {
            lv_label_set_text(ui_home_state_label, "OFF");
            lv_obj_set_style_text_color(ui_home_state_label, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
        }
    }
}

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
            lv_obj_t *label = lv_obj_get_child(ui_stop_button,0);
            if (label) lv_label_set_text(label, "Completed");
lv_obj_set_style_bg_color(ui_stop_button, lv_palette_main(LV_PALETTE_YELLOW), LV_PART_MAIN);
}
}
}void JarController::onTimerTick() {
updateMotor();
updateHomePageUI();
}



