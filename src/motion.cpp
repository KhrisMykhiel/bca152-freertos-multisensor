#include "motion.h"
#include "rtos_objects.h"
#include "driver/gpio.h"

#define PIR_GPIO GPIO_NUM_27
#define MOTION_POLL_MS 200

static volatile bool s_motionFlag = false;

void motionInit() {
    gpio_config_t cfg = {};
    cfg.pin_bit_mask = (1ULL << PIR_GPIO);
    cfg.mode = GPIO_MODE_INPUT;
    cfg.pull_down_en = GPIO_PULLDOWN_ENABLE;
    gpio_config(&cfg);
}

bool motionCurrentlyDetected() {
    return s_motionFlag;
}

// MotionTask polls the PIR line on a short fixed period (Running while
// checking, Blocked while delayed). On a rising edge it sets EVENT_MOTION
// (pulsed, then cleared) so StateTask can react and return the system to
// ACTIVE regardless of current state.
void MotionTask(void *pvParameters) {
    (void)pvParameters;
    bool lastLevel = false;

    for (;;) {
        bool level = gpio_get_level(PIR_GPIO) != 0;
        s_motionFlag = level;

        if (level && !lastLevel) {
            xEventGroupSetBits(systemEventGroup, EVENT_MOTION);
            safePrintf("[MotionTask] motion detected\n");
        }
        lastLevel = level;

        vTaskDelay(pdMS_TO_TICKS(MOTION_POLL_MS));
    }
}
