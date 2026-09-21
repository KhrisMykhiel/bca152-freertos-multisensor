/* Alarm header file */
#ifndef ALARM_H
#define ALARM_H

#include <stdbool.h>

// Pin where the buzzer is connected
#define BUZZER_GPIO_PIN 25

// Limits for temperature
#define LOW_TEMP_THRESHOLD  18.0f
#define HIGH_TEMP_THRESHOLD 30.0f

// States the alarm can be in
typedef enum {
    ALARM_NORMAL,
    ALARM_LOW_TEMPERATURE,
    ALARM_HIGH_TEMPERATURE
} AlarmState;

// Check if temperature is bad
AlarmState evaluateTemperature(float temperature);

#ifndef TEST_NATIVE
// Setup alarm hardware
void initAlarm(void);
// Main loop for the alarm task
void AlarmTask(void* pvParameters);
// Turn the buzzer on or off
void setBuzzer(bool state);
#endif

#endif // ALARM_H
