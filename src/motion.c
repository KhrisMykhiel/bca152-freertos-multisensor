/* Handles the motion sensor checking */
#include "motion.h"
#include "rtos_objects.h"

#ifndef TEST_NATIVE
#include "driver/gpio.h"
#include <stdio.h>

/* Setup the pin connected to the motion sensor */
void initMotion(void) {
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << PIR_GPIO_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
}

/* Check if the motion sensor pin is high or low */
bool readPirRaw(void) {
    // Return true if pin is high, meaning motion is seen
    return gpio_get_level((gpio_num_t)PIR_GPIO_PIN) == 1;
}

/* Task that runs forever to check for motion changes */
void MotionTask(void* pvParameters) {
    (void)pvParameters;
    bool lastMotionState = false; // Save previous state to know when it changes

    for (;;) {
        bool currentMotion = readPirRaw(); // Look at the sensor right now
        
        // If it just started seeing motion, tell the system
        if (currentMotion && !lastMotionState) {
            // Send a signal that motion happened
            xEventGroupSetBits(g_system_event_group, EVENT_MOTION);

            // Print it to the console
            xSemaphoreTake(g_serial_mutex, portMAX_DELAY);
            printf("[MotionTask] PIR motion detected!\n");
            xSemaphoreGive(g_serial_mutex);
        }
        
        // Save the current state for the next check
        lastMotionState = currentMotion;

        // Wait a very short time before checking again
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
#endif
