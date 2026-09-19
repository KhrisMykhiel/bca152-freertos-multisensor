#include "input.h"
#include "display.h"
#include "rtos_objects.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "input";

#define ENC_CLK GPIO_NUM_32
#define ENC_DT  GPIO_NUM_33
#define ENC_SW  GPIO_NUM_26

static TaskHandle_t s_inputTaskHandle = nullptr;

// Encoding of direction sent via task notification value: +1 CW, -1 CCW.
static void IRAM_ATTR encClkIsr(void *arg) {
    int dtLevel = gpio_get_level(ENC_DT);
    int direction = dtLevel ? -1 : 1; // convention depends on wiring
    BaseType_t higherPriorityWoken = pdFALSE;
    // Task notification used here as a lightweight, allocation-free
    // ISR-to-task signal (faster than a queue for a single integer).
    xTaskNotifyFromISR(s_inputTaskHandle, (uint32_t)direction,
                        eSetValueWithOverwrite, &higherPriorityWoken);
    portYIELD_FROM_ISR(higherPriorityWoken);
}

void inputInit() {
    gpio_config_t cfg = {};
    cfg.pin_bit_mask = (1ULL << ENC_CLK) | (1ULL << ENC_DT) | (1ULL << ENC_SW);
    cfg.mode = GPIO_MODE_INPUT;
    cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    cfg.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&cfg);

    gpio_set_intr_type(ENC_CLK, GPIO_INTR_NEGEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(ENC_CLK, encClkIsr, nullptr);

    ESP_LOGI(TAG, "Rotary encoder initialized");
}

void InputTask(void *pvParameters) {
    (void)pvParameters;
    s_inputTaskHandle = xTaskGetCurrentTaskHandle();

    DisplayMode mode = DisplayMode::TEMPERATURE;

    for (;;) {
        uint32_t notifiedValue = 0;
        // Blocks (Blocked state) until the ISR delivers a notification;
        // this is the "process rotary-encoder navigation" responsibility.
        if (xTaskNotifyWait(0, 0xFFFFFFFF, &notifiedValue, portMAX_DELAY) == pdTRUE) {
            int direction = (int32_t)notifiedValue;
            mode = (direction > 0) ? nextDisplayMode(mode) : previousDisplayMode(mode);
            displaySetMode(mode);
            safePrintf("[InputTask] mode changed (dir=%d)\n", direction);
        }
    }
}
