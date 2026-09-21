/* Alarm file to handle buzzer and warnings */
#include "alarm.h"
#include "rtos_objects.h"

#ifndef TEST_NATIVE
#include "driver/gpio.h"
#include "esp_log.h"
#include "sensors.h"
#endif

/* Check if the temperature is too hot, too cold, or normal */
AlarmState evaluateTemperature(float temperature) {
    if (temperature < LOW_TEMP_THRESHOLD) {
        return ALARM_LOW_TEMPERATURE; // It is too cold
    } else if (temperature > HIGH_TEMP_THRESHOLD) {
        return ALARM_HIGH_TEMPERATURE; // It is too hot
    }
    return ALARM_NORMAL; // Temperature is good
}

#ifndef TEST_NATIVE
/* Setup the buzzer pin */
void initAlarm(void) {
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << BUZZER_GPIO_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    
    // Turn off the buzzer at the start
    gpio_set_level((gpio_num_t)BUZZER_GPIO_PIN, 0);
}

/* Turn the buzzer on or off */
void setBuzzer(bool state) {
    gpio_set_level((gpio_num_t)BUZZER_GPIO_PIN, state ? 1 : 0);
}

/* Task that runs forever to check if we should ring the alarm */
void AlarmTask(void* pvParameters) {
    (void)pvParameters;
    SensorData data;
    bool buzzerState = false;

    for (;;) {
        // Look at the latest sensor data without taking it out of the queue
        if (xQueuePeek(g_sensor_queue, &data, portMAX_DELAY) == pdTRUE) {
            AlarmState state = evaluateTemperature(data.temperature);
            
            // Check if the system is awake
            EventBits_t bits = xEventGroupGetBits(g_system_event_group);
            bool isActive = (bits & EVENT_ACTIVE) != 0;

            // If awake and temperature is bad, make sound
            if (isActive && state != ALARM_NORMAL) {
                xEventGroupSetBits(g_system_event_group, EVENT_ALARM);
                
                // Toggle the buzzer to make a beeping sound
                buzzerState = !buzzerState;
                setBuzzer(buzzerState);
            } else {
                // Clear the alarm and turn off the buzzer
                xEventGroupClearBits(g_system_event_group, EVENT_ALARM);
                buzzerState = false;
                setBuzzer(false);
            }
        }
        
        // Wait a short time before checking again
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}
#endif
