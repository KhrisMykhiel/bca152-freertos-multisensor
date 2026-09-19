#include "rtos_objects.h"
#include "sensors.h"
#include "display.h"
#include "input.h"
#include "motion.h"
#include "alarm.h"
#include "system_state.h"

#include "esp_log.h"

static const char *TAG = "main";

extern "C" void app_main() {
    ESP_LOGI(TAG, "BCA152 FreeRTOS Multisensor");
    ESP_LOGI(TAG, "System starting...");

    // ---- 1. Hardware initialization -----------------------------------
    sensorsInit();
    displayInit();
    inputInit();
    motionInit();
    alarmInit();

    // ---- 2. FreeRTOS object creation -----------------------------------
    rtosObjectsInit();

    // ---- 3. Task creation ------------------------------------------------
    xTaskCreate(SensorTask,  "SensorTask",  STACK_SENSOR,  nullptr, PRIORITY_SENSOR,  nullptr);
    xTaskCreate(DisplayTask, "DisplayTask", STACK_DISPLAY, nullptr, PRIORITY_DISPLAY, nullptr);
    xTaskCreate(InputTask,   "InputTask",   STACK_INPUT,   nullptr, PRIORITY_INPUT,   nullptr);
    xTaskCreate(MotionTask,  "MotionTask",  STACK_MOTION,  nullptr, PRIORITY_MOTION,  nullptr);
    xTaskCreate(AlarmTask,   "AlarmTask",   STACK_ALARM,   nullptr, PRIORITY_ALARM,   nullptr);
    xTaskCreate(StateTask,   "StateTask",   STACK_STATE,   nullptr, PRIORITY_STATE,   nullptr);

    // ---- 4. Scheduler-driven operation from here on ---------------------
    ESP_LOGI(TAG, "All tasks created, scheduler running");
}
