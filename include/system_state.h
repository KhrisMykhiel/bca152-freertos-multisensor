#pragma once
#include <stdint.h>

enum class SystemState {
    ACTIVE,
    INACTIVE
};

// Pure, unit-testable transition function.
//   currentState        : state before this evaluation
//   motionSeen           : true if motion occurred since last check
//   msSinceLastMotion    : elapsed time since motion was last seen
//   inactivityTimeoutMs  : configured timeout (see INACTIVITY_TIMEOUT_MS)
SystemState evaluateSystemState(SystemState currentState, bool motionSeen,
                                 uint32_t msSinceLastMotion,
                                 uint32_t inactivityTimeoutMs);

void StateTask(void *pvParameters);
