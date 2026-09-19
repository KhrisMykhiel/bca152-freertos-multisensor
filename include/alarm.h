#pragma once

enum class AlarmState {
    NORMAL,
    LOW_TEMPERATURE,
    HIGH_TEMPERATURE
};

// Pure function: no hardware access, fully unit-testable without Wokwi/ESP32.
// This is the "hardware-independent decision logic" required by the lab.
AlarmState evaluateTemperature(float temperature);

void alarmInit();          // configures the buzzer GPIO/LEDC
void AlarmTask(void *pvParameters);
