#include "system_state.h"
#include "rtos_objects.h"

SystemState evaluateSystemState(SystemState currentState, bool motionSeen,
                                 uint32_t msSinceLastMotion,
                                 uint32_t inactivityTimeoutMs) {
    if (motionSeen) {
        return SystemState::ACTIVE;
    }
    if (currentState == SystemState::ACTIVE &&
        msSinceLastMotion >= inactivityTimeoutMs) {
        return SystemState::INACTIVE;
    }
    return currentState; // no change
}

// StateTask is the single writer of EVENT_ACTIVE, centralizing
// ACTIVE/INACTIVE management as recommended by the lab spec so no other
// task independently decides system activity state.
void StateTask(void *pvParameters) {
    (void)pvParameters;
    SystemState state = SystemState::ACTIVE;
    TickType_t lastMotionTick = xTaskGetTickCount();

    const TickType_t pollPeriod = pdMS_TO_TICKS(500);
    TickType_t lastWakeTime = xTaskGetTickCount();

    for (;;) {
        // Clearing on read lets us detect "motion happened since last
        // check" without missing pulses between polls.
        EventBits_t bits = xEventGroupClearBits(systemEventGroup, EVENT_MOTION);
        bool motionSeen = (bits & EVENT_MOTION) != 0;

        if (motionSeen) {
            lastMotionTick = xTaskGetTickCount();
        }
        uint32_t msSinceMotion =
            (xTaskGetTickCount() - lastMotionTick) * portTICK_PERIOD_MS;

        SystemState next = evaluateSystemState(state, motionSeen, msSinceMotion,
                                                INACTIVITY_TIMEOUT_MS);
        if (next != state) {
            safePrintf("[StateTask] %s -> %s\n",
                       state == SystemState::ACTIVE ? "ACTIVE" : "INACTIVE",
                       next == SystemState::ACTIVE ? "ACTIVE" : "INACTIVE");
            state = next;
            if (state == SystemState::ACTIVE) {
                xEventGroupSetBits(systemEventGroup, EVENT_ACTIVE);
            } else {
                xEventGroupClearBits(systemEventGroup, EVENT_ACTIVE);
            }
        }

        vTaskDelayUntil(&lastWakeTime, pollPeriod);
    }
}
