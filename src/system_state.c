/* This file handles if the system is awake or asleep based on motion */
#include "system_state.h"
#include "rtos_objects.h"

#ifndef TEST_NATIVE
#include "esp_log.h"
#include <stdio.h>
#endif

/* Decide the next state of the system */
SystemState evaluateSystemState(SystemState current, bool motionDetected, uint32_t elapsedInactiveSec, uint32_t timeoutSec) {
    if (current == SYSTEM_ACTIVE) {
        // Go to sleep if no one is moving for a long time
        if (!motionDetected && elapsedInactiveSec >= timeoutSec) {
            return SYSTEM_INACTIVE;
        }
        return SYSTEM_ACTIVE;
    } else {
        // Wake up if someone moves
        if (motionDetected) {
            return SYSTEM_ACTIVE;
        }
        return SYSTEM_INACTIVE;
    }
}

#ifndef TEST_NATIVE
/* Task that runs forever to check sleep and wake state */
void StateTask(void* pvParameters) {
    (void)pvParameters;
    SystemState currentState = SYSTEM_ACTIVE;
    uint32_t elapsedInactiveSec = 0; // Time since last movement

    // Start as awake
    xEventGroupSetBits(g_system_event_group, EVENT_ACTIVE);

    for (;;) {
        // Wait for movement for up to 1 second
        EventBits_t bits = xEventGroupWaitBits(
            g_system_event_group,
            EVENT_MOTION,
            pdTRUE,      
            pdFALSE,      
            pdMS_TO_TICKS(1000) 
        );

        bool motionDetected = (bits & EVENT_MOTION) != 0;

        if (motionDetected) {
            // Reset the timer if someone moves
            elapsedInactiveSec = 0;
        } else if (currentState == SYSTEM_ACTIVE) {
            // Count seconds when there is no movement
            elapsedInactiveSec++;
        }

        // Figure out if we should sleep or stay awake
        SystemState nextState = evaluateSystemState(
            currentState,
            motionDetected,
            elapsedInactiveSec,
            INACTIVITY_TIMEOUT_SECONDS
        );

        // If the state changes, update the system and print a message
        if (nextState != currentState) {
            currentState = nextState;
            if (currentState == SYSTEM_ACTIVE) {
                // Wake up
                xEventGroupSetBits(g_system_event_group, EVENT_ACTIVE);
                xSemaphoreTake(g_serial_mutex, portMAX_DELAY);
                printf("[StateTask] Transitioned to state: ACTIVE\n");
                xSemaphoreGive(g_serial_mutex);
            } else {
                // Go to sleep
                xEventGroupClearBits(g_system_event_group, EVENT_ACTIVE);
                xSemaphoreTake(g_serial_mutex, portMAX_DELAY);
                printf("[StateTask] Inactivity timeout reached. Transitioned to state: INACTIVE\n");
                xSemaphoreGive(g_serial_mutex);
            }
        }
    }
}
#endif
