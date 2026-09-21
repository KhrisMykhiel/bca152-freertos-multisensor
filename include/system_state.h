/* System state header file */
#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <stdint.h>
#include <stdbool.h>

// Seconds to wait before going to sleep
#define INACTIVITY_TIMEOUT_SECONDS 15

// Sleep states
typedef enum {
    SYSTEM_ACTIVE, // Awake
    SYSTEM_INACTIVE // Asleep
} SystemState;

// Decide the next sleep state
SystemState evaluateSystemState(SystemState current, bool motionDetected, uint32_t elapsedInactiveSec, uint32_t timeoutSec);

#ifndef TEST_NATIVE
// Main loop to check if we should sleep or wake up
void StateTask(void* pvParameters);
#endif

#endif // SYSTEM_STATE_H
