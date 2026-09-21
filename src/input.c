/* Handles reading the knob (rotary encoder) to change screen pages */
#include "input.h"
#include "rtos_objects.h"

#ifndef TEST_NATIVE
#include <stdio.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

static const char* TAG = "Input";
// A special queue to pass knob turning events
static QueueHandle_t s_encoder_queue = NULL;

/* This special function is called exactly when the knob is turned */
static void IRAM_ATTR encoder_isr_handler(void* arg) {
    (void)arg;
    static int64_t s_last_interrupt_time = 0;
    int64_t now = esp_timer_get_time();

    // Ignore bounces by checking if enough time passed since the last turn
    if (now - s_last_interrupt_time > 15000) {
        s_last_interrupt_time = now;
        
        // Read the other pin to see which way the knob was turned
        int dtVal = gpio_get_level((gpio_num_t)ENCODER_DT_PIN);
        int step = (dtVal != 0) ? 1 : -1;

        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (s_encoder_queue != NULL) {
            // Send the turn direction to the main task
            xQueueSendFromISR(s_encoder_queue, &step, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}
#endif

/* Figure out the next page to show when turning right */
DisplayMode nextDisplayMode(DisplayMode current) {
    switch (current) {
        case DISPLAY_TEMPERATURE: return DISPLAY_HUMIDITY;
        case DISPLAY_HUMIDITY:    return DISPLAY_LIGHT;
        case DISPLAY_LIGHT:       return DISPLAY_MOTION;
        case DISPLAY_MOTION:      return DISPLAY_TEMPERATURE;
        default:                  return DISPLAY_TEMPERATURE;
    }
}

/* Figure out the previous page to show when turning left */
DisplayMode previousDisplayMode(DisplayMode current) {
    switch (current) {
        case DISPLAY_TEMPERATURE: return DISPLAY_MOTION;
        case DISPLAY_HUMIDITY:    return DISPLAY_TEMPERATURE;
        case DISPLAY_LIGHT:       return DISPLAY_HUMIDITY;
        case DISPLAY_MOTION:      return DISPLAY_LIGHT;
        default:                  return DISPLAY_TEMPERATURE;
    }
}

#ifndef TEST_NATIVE
/* Setup the pins and hardware for the knob */
void initInput(void) {
    if (s_encoder_queue == NULL) {
        s_encoder_queue = xQueueCreate(10, sizeof(int));
    }

    // Configure the main knob turning pin to trigger our special function when it changes
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << ENCODER_CLK_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    // Configure the direction pin and the button pin
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = (1ULL << ENCODER_DT_PIN) | (1ULL << ENCODER_SW_PIN);
    gpio_config(&io_conf);

    // Turn on the hardware feature that handles these pin changes
    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to setup pin service: %d", err);
    }

    // Connect our special function to the knob turning pin
    gpio_isr_handler_add((gpio_num_t)ENCODER_CLK_PIN, encoder_isr_handler, NULL);
    ESP_LOGI(TAG, "Knob setup complete on CLK=%d, DT=%d", ENCODER_CLK_PIN, ENCODER_DT_PIN);
}

/* Task that runs forever to update the screen page when the knob is turned */
void InputTask(void* pvParameters) {
    (void)pvParameters;
    
    // Start by showing the temperature
    DisplayMode currentMode = DISPLAY_TEMPERATURE;
    xQueueOverwrite(g_display_queue, &currentMode);

    int step = 0;
    for (;;) {
        // Wait here until the knob is turned
        if (xQueueReceive(s_encoder_queue, &step, portMAX_DELAY) == pdTRUE) {
            // Change the page depending on the turn direction
            if (step > 0) {
                currentMode = nextDisplayMode(currentMode);
            } else if (step < 0) {
                currentMode = previousDisplayMode(currentMode);
            }
            
            // Tell the display task to show the new page
            xQueueOverwrite(g_display_queue, &currentMode);

            // Print the new page number to the console
            xSemaphoreTake(g_serial_mutex, portMAX_DELAY);
            printf("[InputTask] Display page changed to: %d\n", (int)currentMode);
            xSemaphoreGive(g_serial_mutex);
        }
    }
}
#endif
