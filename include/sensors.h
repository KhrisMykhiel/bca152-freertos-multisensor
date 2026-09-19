#pragma once
#include <stdbool.h>

// Shared sensor snapshot pushed onto sensorQueue every sampling period.
struct SensorData {
    float temperature;   // Celsius
    float humidity;      // percent RH
    int   lightLevel;    // 0-100 (documented raw-ADC -> percent mapping)
    bool  motionDetected;
};

// GPIO/ADC init for DHT22 + LDR. Called once from app_main before tasks start.
void sensorsInit();

// FreeRTOS task: periodically reads DHT22 + LDR, merges in the latest
// motion flag, and pushes a SensorData sample onto sensorQueue.
// Uses vTaskDelayUntil() to keep a fixed 2 s period regardless of how long
// the read takes (see report section on drift).
void SensorTask(void *pvParameters);

// Low-level reads, exposed for unit testing / reuse.
bool readDHT22(float *temperatureOut, float *humidityOut);
int  readLightPercent();
