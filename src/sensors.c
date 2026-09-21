/* Handles reading data from the room sensors */
#include "sensors.h"
#include "rtos_objects.h"
#include "motion.h"

#ifndef TEST_NATIVE
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

static adc_oneshot_unit_handle_t s_adc1_handle = NULL;
#endif

/* Convert raw light sensor reading to a percentage */
int convertRawAdcToLightPercent(int rawAdc) {
    if (rawAdc < 0) rawAdc = 0;
    if (rawAdc > 4095) rawAdc = 4095;
    return (int)(((4095 - rawAdc) * 100) / 4095);
}

#ifndef TEST_NATIVE
/* Setup the light and temperature sensors */
void initSensors(void) {
    adc_oneshot_unit_init_cfg_t init_config1 = {};
    init_config1.unit_id = ADC_UNIT_1;
    init_config1.ulp_mode = ADC_ULP_MODE_DISABLE;
    
    // Create connection to read light sensor
    esp_err_t err = adc_oneshot_new_unit(&init_config1, &s_adc1_handle);
    if (err != ESP_OK) {
        printf("[Sensors] Error initializing ADC1 unit: %d\n", err);
    }

    // Configure the pin for light sensor
    adc_oneshot_chan_cfg_t chan_config = {};
    chan_config.atten = ADC_ATTEN_DB_12;
    chan_config.bitwidth = ADC_BITWIDTH_DEFAULT;
    err = adc_oneshot_config_channel(s_adc1_handle, ADC_CHANNEL_6, &chan_config);
    if (err != ESP_OK) {
        printf("[Sensors] Error configuring ADC1 channel 6: %d\n", err);
    }

    // Configure the pin for the temperature and humidity sensor
    gpio_config_t dht_conf = {};
    dht_conf.intr_type = GPIO_INTR_DISABLE;
    dht_conf.mode = GPIO_MODE_INPUT;
    dht_conf.pin_bit_mask = (1ULL << DHT_GPIO_PIN);
    dht_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    dht_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&dht_conf);
}

/* Read data from the DHT22 temperature and humidity sensor */
static bool readDHT22(float* tempOut, float* humOut) {
    uint8_t data[5] = {0, 0, 0, 0, 0};

    // Send start signal to the sensor
    gpio_set_direction((gpio_num_t)DHT_GPIO_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)DHT_GPIO_PIN, 0);
    ets_delay_us(2000);
    gpio_set_level((gpio_num_t)DHT_GPIO_PIN, 1);
    ets_delay_us(30);
    gpio_set_direction((gpio_num_t)DHT_GPIO_PIN, GPIO_MODE_INPUT);

    // Wait for sensor to reply
    int timeout = 100;
    while (gpio_get_level((gpio_num_t)DHT_GPIO_PIN) == 1) {
        if (--timeout == 0) return false;
        ets_delay_us(1);
    }
    timeout = 100;
    while (gpio_get_level((gpio_num_t)DHT_GPIO_PIN) == 0) {
        if (--timeout == 0) return false;
        ets_delay_us(1);
    }
    timeout = 100;
    while (gpio_get_level((gpio_num_t)DHT_GPIO_PIN) == 1) {
        if (--timeout == 0) return false;
        ets_delay_us(1);
    }

    // Read the bits from the sensor
    for (int i = 0; i < 40; ++i) {
        timeout = 100;
        while (gpio_get_level((gpio_num_t)DHT_GPIO_PIN) == 0) {
            if (--timeout == 0) return false;
            ets_delay_us(1);
        }
        int64_t start = esp_timer_get_time();
        timeout = 100;
        while (gpio_get_level((gpio_num_t)DHT_GPIO_PIN) == 1) {
            if (--timeout == 0) return false;
            ets_delay_us(1);
        }
        int64_t duration = esp_timer_get_time() - start;
        if (duration > 40) {
            data[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    // Check if the data is valid
    if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        return false; // Data is bad
    }

    // Convert data to human readable numbers
    float rawHum = ((data[0] << 8) | data[1]) * 0.1f;
    float rawTemp = (((data[2] & 0x7F) << 8) | data[3]) * 0.1f;
    if (data[2] & 0x80) {
        rawTemp = -rawTemp;
    }

    *humOut = rawHum;
    *tempOut = rawTemp;
    return true; // Read success
}

/* Read raw value from light sensor */
static int readLDR(void) {
    int rawValue = 0;
    if (s_adc1_handle != NULL) {
        adc_oneshot_read(s_adc1_handle, ADC_CHANNEL_6, &rawValue);
    }
    return rawValue;
}

/* Task that runs forever to read all sensors */
void SensorTask(void* pvParameters) {
    (void)pvParameters;
    TickType_t lastWakeTime = xTaskGetTickCount();
    
    // Set some starting fake values
    SensorData data = { 24.5f, 55.0f, 50, false };

    for (;;) {
        float t = 0.0f, h = 0.0f;
        
        // Read temperature and humidity
        if (readDHT22(&t, &h)) {
            data.temperature = t;
            data.humidity = h;
        }

        // Read light level
        int rawAdc = readLDR();
        data.lightLevel = convertRawAdcToLightPercent(rawAdc);

        // Check if motion sensor sees something
        data.motionDetected = readPirRaw();

        // Share the new data with other parts of the system
        xQueueOverwrite(g_sensor_queue, &data);

        // Print the readings to the console
        xSemaphoreTake(g_serial_mutex, portMAX_DELAY);
        printf("[SensorTask] Temperature: %.2f C, Humidity: %.2f %%, Light: %d %%\n",
               data.temperature, data.humidity, data.lightLevel);
        xSemaphoreGive(g_serial_mutex);

        // Wait 2 seconds before reading again
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(2000));
    }
}
#endif
