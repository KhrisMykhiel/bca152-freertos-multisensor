/* Shared tools header file */
#ifndef RTOS_OBJECTS_H
#define RTOS_OBJECTS_H

#ifndef TEST_NATIVE
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#endif

// Flags for system events
#define EVENT_ACTIVE BIT0 // System is awake
#define EVENT_MOTION BIT1 // Motion was seen
#define EVENT_ALARM  BIT2 // Alarm is ringing

#ifndef TEST_NATIVE
// Mailbox for sensor data
extern QueueHandle_t g_sensor_queue;
// Mailbox for the screen page
extern QueueHandle_t g_display_queue;
// Lock for printing messages safely
extern SemaphoreHandle_t g_serial_mutex;
// Bulletin board for flags
extern EventGroupHandle_t g_system_event_group;

// Print a message safely
void safe_serial_print(const char* message);

// Create the shared tools
void initRtosObjects(void);
#endif

#endif // RTOS_OBJECTS_H
