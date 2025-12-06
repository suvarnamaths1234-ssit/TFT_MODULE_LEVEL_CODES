#ifndef JAR_CONTROLLER_H
#define JAR_CONTROLLER_H

#include <Arduino.h>
#include <lvgl.h>

class JarController {
private:
    int jarID;
    int setRPM;
    int setDuration;
    bool directionC;
    bool motorOn;
    bool settingsSaved;
    bool directionCW;
    
    unsigned long motorStartMillis;
    unsigned long elapsedMillis;
    
    int motorEnablePin;
    int motorDirectionPin;
    int pwmChannel;
    
    // Home page UI objects (INDIVIDUAL for each jar)
    lv_obj_t *ui_home_countdown_label;
    lv_obj_t *ui_home_state_label;
    
    // Info page UI objects (SHARED - only for input, NOT for display)
    lv_obj_t *ui_rpm_textarea;
    lv_obj_t *ui_duration_textarea;
    lv_obj_t *ui_save_button;
    lv_obj_t *ui_start_button;
    lv_obj_t *ui_stop_button;
    lv_obj_t *ui_direction_switch;
    
    // Info page DISPLAY objects (separate - updated by main.ino, not by jar)
    lv_obj_t *ui_rpm_reading_label;
    lv_obj_t *ui_timeleft_label;
    
public:
    JarController(int id, int enablePin, int directionPin, int channel);
    
    void begin();
    
    // Motor Control
    void startMotor();
    void stopMotor();
    void updateMotor();
    
    // Settings Management
    void setRPMValue(int rpm);
    void setDurationValue(int duration);
    void setDirection(bool clockwise);
    int getRPM() const { return setRPM; }
    int getDuration() const { return setDuration; }
    bool isMotorRunning() const { return motorOn; }
    bool areSettingsSaved() const { return settingsSaved; }
    bool getDirection() const { return directionCW; }
    unsigned long getRemainingTime() const;
    
    // Increment/Decrement
    void incrementRPM(int step = 5);
    void decrementRPM(int step = 5);
    void incrementDuration(int step = 10);
    void decrementDuration(int step = 10);
    
    // Save/Load
    bool saveSettings(const char *filepath);
    
    // UI Binding - Info Page (shared, input only)
    void bindInfoPageUI(lv_obj_t *rpm_ta, lv_obj_t *dur_ta,
                        lv_obj_t *save_btn, lv_obj_t *start_btn, lv_obj_t *stop_btn,
                        lv_obj_t *dir_switch);
    
    // UI Binding - Home Page (individual for each jar)
    void bindHomePageUI(lv_obj_t *home_countdown, lv_obj_t *home_state);
    
    // UI Update Functions
    void updateUIFields();           // Update info page inputs only
    void updateHomePageUI();         // Update home page for this jar
    void updateButtonStates(bool running);
    
    void onTimerTick();
    void markUnsaved();


        

};

#endif