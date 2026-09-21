/* Sets up the shared tools that help tasks talk to each other */
#include "rtos_objects.h"
#include "sensors.h"
#include "input.h"
#include <stdio.h>

#ifndef TEST_NATIVE
// Mailbox for sending sensor readings to the display and alarm
QueueHandle_t g_sensor_queue = NULL;

// Mailbox for sending the current screen page to the display
QueueHandle_t g_display_queue = NULL;

// A lock to make sure only one task prints to the screen at a time
SemaphoreHandle_t g_serial_mutex = NULL;

// A bulletin board to post flags like "motion happened" or "alarm is ringing"
EventGroupHandle_t g_system_event_group = NULL;

/* Create all the shared tools */
void initRtosObjects(void) {
    // Create a mailbox that holds one set of sensor readings
    g_sensor_queue = xQueueCreate(1, sizeof(SensorData));

    // Create a mailbox that holds one page number
    g_display_queue = xQueueCreate(1, sizeof(DisplayMode));

    // Create the lock for printing messages
    g_serial_mutex = xSemaphoreCreateMutex();

    // Create the bulletin board for flags
    g_system_event_group = xEventGroupCreate();

    // Set the system to start as awake
    xEventGroupSetBits(g_system_event_group, EVENT_ACTIVE);
}

/* Print a message safely without mixing up letters from different tasks */
void safe_serial_print(const char* message) {
    // Only use the lock if it exists
    if (g_serial_mutex != NULL) {
        // Wait to get the lock, print, and then give the lock back
        if (xSemaphoreTake(g_serial_mutex, portMAX_DELAY) == pdTRUE) {
            printf("%s", message);
            xSemaphoreGive(g_serial_mutex);
        }
    } else {
        // If there is no lock, just print normally
        printf("%s", message);
    }
}
#endif
