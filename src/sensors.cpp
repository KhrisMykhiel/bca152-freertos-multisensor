#include "sensors.h"
#include "rtos_objects.h"
#include "motion.h"

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_rom_sys.h"   // esp_rom_delay_us
#include "esp_log.h"

static const char *TAG = "sensors";

#define DHT22_GPIO        GPIO_NUM_4
#define LDR_ADC_CHANNEL    ADC_CHANNEL_6   // GPIO34 on most devkits
static adc_oneshot_unit_handle_t s_adcHandle = nullptr;

void sensorsInit() {
    // LDR via ADC1 one-shot driver.
    adc_oneshot_unit_init_cfg_t initCfg = {};
initCfg.unit_id = ADC_UNIT_1;       
    adc_oneshot_new_unit(&initCfg, &s_adcHandle);

    adc_oneshot_chan_cfg_t chanCfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    adc_oneshot_config_channel(s_adcHandle, LDR_ADC_CHANNEL, &chanCfg);

    // DHT22 data line: idle high, driven briefly low by us to start a
    // transaction, then read back as open-drain input.
    gpio_config_t dhtCfg = {};
    dhtCfg.pin_bit_mask = (1ULL << DHT22_GPIO);
    dhtCfg.mode = GPIO_MODE_INPUT_OUTPUT_OD;
    dhtCfg.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&dhtCfg);
    gpio_set_level(DHT22_GPIO, 1);

    ESP_LOGI(TAG, "Sensors initialized (DHT22 on GPIO%d, LDR on ADC1 ch%d)",
             DHT22_GPIO, LDR_ADC_CHANNEL);
}

// Minimal bit-banged DHT22 read. Returns false on timeout/checksum failure
// (caller should hold last-known-good value rather than trust a bad frame).
bool readDHT22(float *temperatureOut, float *humidityOut) {
    uint8_t data[5] = {0, 0, 0, 0, 0};

    // Start signal: pull low >=1ms, release, then read the sensor's response.
    gpio_set_level(DHT22_GPIO, 0);
    esp_rom_delay_us(1200);
    gpio_set_level(DHT22_GPIO, 1);
    esp_rom_delay_us(30);

    // Wait for sensor's ACK (low ~80us, high ~80us).
    int timeout = 0;
    while (gpio_get_level(DHT22_GPIO) == 1) {
        if (++timeout > 100) return false;
        esp_rom_delay_us(1);
    }
    timeout = 0;
    while (gpio_get_level(DHT22_GPIO) == 0) {
        if (++timeout > 100) return false;
        esp_rom_delay_us(1);
    }
    timeout = 0;
    while (gpio_get_level(DHT22_GPIO) == 1) {
        if (++timeout > 100) return false;
        esp_rom_delay_us(1);
    }

    // Read 40 bits. Each bit: ~50us low, then high for ~26-28us (a 0) or
    // ~70us (a 1).
    for (int i = 0; i < 40; i++) {
        timeout = 0;
        while (gpio_get_level(DHT22_GPIO) == 0) {
            if (++timeout > 100) return false;
            esp_rom_delay_us(1);
        }
        esp_rom_delay_us(35); // sample partway into the high pulse
        int bit = gpio_get_level(DHT22_GPIO) ? 1 : 0;
        timeout = 0;
        while (gpio_get_level(DHT22_GPIO) == 1) {
            if (++timeout > 100) return false;
            esp_rom_delay_us(1);
        }
        data[i / 8] <<= 1;
        data[i / 8] |= bit;
    }

    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        ESP_LOGW(TAG, "DHT22 checksum mismatch");
        return false;
    }

    uint16_t rawHumidity = (data[0] << 8) | data[1];
    uint16_t rawTempRaw  = (data[2] << 8) | data[3];
    bool negative = rawTempRaw & 0x8000;
    uint16_t rawTemp = rawTempRaw & 0x7FFF;

    *humidityOut = rawHumidity / 10.0f;
    *temperatureOut = negative ? -(rawTemp / 10.0f) : (rawTemp / 10.0f);
    return true;
}

// Converts raw ADC counts (0-4095 on a 12-bit read) to a documented 0-100%
// scale. This is a relative light level, NOT a calibrated lux value.
int readLightPercent() {
    int raw = 0;
    adc_oneshot_read(s_adcHandle, LDR_ADC_CHANNEL, &raw);
    int percent = (raw * 100) / 4095;
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    return percent;
}

void SensorTask(void *pvParameters) {
    (void)pvParameters;
    SensorData lastGood = {25.0f, 50.0f, 50, false};

    TickType_t lastWakeTime = xTaskGetTickCount();
    for (;;) {
        float temperature, humidity;
        if (readDHT22(&temperature, &humidity)) {
            lastGood.temperature = temperature;
            lastGood.humidity = humidity;
        } else {
            safePrintf("[SensorTask] DHT22 read failed, reusing last value\n");
        }

        lastGood.lightLevel = readLightPercent();
        lastGood.motionDetected = motionCurrentlyDetected();

        // Post to both consumer queues. Drop-oldest-on-full keeps each
        // queue from backing up if a consumer is briefly slow, since only
        // the latest reading is ever useful to Display/Alarm.
        if (xQueueSend(sensorQueueDisplay, &lastGood, 0) != pdTRUE) {
            SensorData discard;
            xQueueReceive(sensorQueueDisplay, &discard, 0);
            xQueueSend(sensorQueueDisplay, &lastGood, 0);
        }
        if (xQueueSend(sensorQueueAlarm, &lastGood, 0) != pdTRUE) {
            SensorData discard;
            xQueueReceive(sensorQueueAlarm, &discard, 0);
            xQueueSend(sensorQueueAlarm, &lastGood, 0);
        }

        safePrintf("[SensorTask] T=%.2fC H=%.2f%% L=%d%% M=%d\n",
                   lastGood.temperature, lastGood.humidity,
                   lastGood.lightLevel, lastGood.motionDetected);

        // vTaskDelayUntil keeps the sampling period fixed at exactly
        // SENSOR_PERIOD_MS regardless of how long the DHT22 bit-bang read
        // took this cycle, preventing cumulative drift (see report).
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(SENSOR_PERIOD_MS));
    }
}
