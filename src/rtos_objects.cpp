#include "rtos_objects.h"
#include "sensors.h"
#include <stdio.h>
#include <stdarg.h>

QueueHandle_t sensorQueueDisplay = nullptr;
QueueHandle_t sensorQueueAlarm = nullptr;
SemaphoreHandle_t serialMutex = nullptr;
EventGroupHandle_t systemEventGroup = nullptr;

void rtosObjectsInit() {
    sensorQueueDisplay = xQueueCreate(SENSOR_QUEUE_LENGTH, sizeof(SensorData));
    configASSERT(sensorQueueDisplay != nullptr);

    sensorQueueAlarm = xQueueCreate(SENSOR_QUEUE_LENGTH, sizeof(SensorData));
    configASSERT(sensorQueueAlarm != nullptr);

    serialMutex = xSemaphoreCreateMutex();
    configASSERT(serialMutex != nullptr);

    systemEventGroup = xEventGroupCreate();
    configASSERT(systemEventGroup != nullptr);

    // System starts ACTIVE.
    xEventGroupSetBits(systemEventGroup, EVENT_ACTIVE);
}

void safePrintf(const char *fmt, ...) {
    // Mutex protects the shared UART resource. Without it, two tasks
    // printing at the same tick would interleave characters mid-line
    // (see PART XI in the lab spec / Fault Experiment 3).
    if (serialMutex == nullptr) {
        return; // not yet initialized; drop silently
    }
    if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
        xSemaphoreGive(serialMutex);
    }
}
