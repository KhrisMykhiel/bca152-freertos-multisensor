/* Sensors header file */
#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>
#include <stdbool.h>

// Pin for temperature and humidity sensor
#define DHT_GPIO_PIN 4
// Pin for light sensor
#define LDR_ADC_CHANNEL ADC_CHANNEL_6

// Structure to hold all sensor readings
typedef struct {
    float temperature;
    float humidity;
    int lightLevel;
    bool motionDetected;
} SensorData;

// Change raw light sensor number into a percentage
int convertRawAdcToLightPercent(int rawAdc);

#ifndef TEST_NATIVE
// Setup all sensors
void initSensors(void);
// Main loop for the sensor task
void SensorTask(void* pvParameters);
#endif

#endif // SENSORS_H
