/* Main entry point for the Room Monitoring System */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "rtos_objects.h"
#include "sensors.h"
#include "display.h"
#include "input.h"
#include "alarm.h"
#include "motion.h"
#include "system_state.h"

/* Main function that starts everything */
void app_main(void) {
    // Print a welcome message when the system turns on
    printf("\n/* BCA152 FreeRTOS Multisensor */\n");
    printf("System starting...\n\n");

    // Create shared items like queues and locks
    initRtosObjects();

    // Setup all hardware parts
    initSensors();
    initDisplay();
    initInput();
    initAlarm();
    initMotion();

    // Start the motion task to check for movement (High priority)
    xTaskCreate(
        MotionTask,
        "MotionTask",
        3072,
        NULL,
        3,
        NULL
    );

    // Start the input task to read buttons or knobs (High priority)
    xTaskCreate(
        InputTask,
        "InputTask",
        3072,
        NULL,
        3,
        NULL
    );

    // Start the sensor task to read temperature and humidity (Medium priority)
    xTaskCreate(
        SensorTask,
        "SensorTask",
        4096,
        NULL,
        2,
        NULL
    );

    // Start the alarm task to beep if things go wrong (Medium priority)
    xTaskCreate(
        AlarmTask,
        "AlarmTask",
        3072,
        NULL,
        2,
        NULL
    );

    // Start the state task to manage sleep or awake modes (Medium priority)
    xTaskCreate(
        StateTask,
        "StateTask",
        3072,
        NULL,
        2,
        NULL
    );

    // Start the display task to show information on the screen (Low priority)
    xTaskCreate(
        DisplayTask,
        "DisplayTask",
        4096,
        NULL,
        1,
        NULL
    );

    // Tell the user that everything is ready
    safe_serial_print("[System] All parts are ready and running.\n");
}
