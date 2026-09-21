#include "esp_log.h"

extern int main(int argc, char **argv);

void app_main(void) {
    ESP_LOGI("test_runner", "Running unit tests on ESP32 target...");
    main(0, NULL);
}