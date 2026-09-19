#include "alarm.h"
#include "rtos_objects.h"
#include "sensors.h"
#include "driver/gpio.h"

#define BUZZER_GPIO GPIO_NUM_25

AlarmState evaluateTemperature(float temperature) {
    if (temperature < LOW_TEMPERATURE_C) {
        return AlarmState::LOW_TEMPERATURE;
    }
    if (temperature > HIGH_TEMPERATURE_C) {
        return AlarmState::HIGH_TEMPERATURE;
    }
    return AlarmState::NORMAL;
}

void alarmInit() {
    gpio_config_t cfg = {};
    cfg.pin_bit_mask = (1ULL << BUZZER_GPIO);
    cfg.mode = GPIO_MODE_OUTPUT;
    gpio_config(&cfg);
    gpio_set_level(BUZZER_GPIO, 0);
}

// AlarmTask blocks on the sensor queue (waiting-for-data), evaluates the
// pure decision function, drives the buzzer GPIO, and mirrors the result
// into the EVENT_ALARM bit so other tasks (e.g. DisplayTask) can react
// without needing direct access to alarm internals.
void AlarmTask(void *pvParameters) {
    (void)pvParameters;
    SensorData sample;
    AlarmState lastState = AlarmState::NORMAL;

    for (;;) {
        if (xQueueReceive(sensorQueueAlarm, &sample, portMAX_DELAY) == pdTRUE) {
            AlarmState state = evaluateTemperature(sample.temperature);

            if (state != lastState) {
                safePrintf("[AlarmTask] state change %d -> %d (T=%.2fC)\n",
                           (int)lastState, (int)state, sample.temperature);
                lastState = state;
            }

            bool alarmActive = (state != AlarmState::NORMAL);
            gpio_set_level(BUZZER_GPIO, alarmActive ? 1 : 0);

            if (alarmActive) {
                xEventGroupSetBits(systemEventGroup, EVENT_ALARM);
            } else {
                xEventGroupClearBits(systemEventGroup, EVENT_ALARM);
            }
        }
    }
}
