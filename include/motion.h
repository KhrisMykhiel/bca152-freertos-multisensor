/* Motion sensor header file */
#ifndef MOTION_H
#define MOTION_H

#include <stdbool.h>

// Pin where the motion sensor is connected
#define PIR_GPIO_PIN 27

#ifndef TEST_NATIVE
// Setup motion sensor
void initMotion(void);
// Main loop for motion task
void MotionTask(void* pvParameters);
// Read the sensor value directly
bool readPirRaw(void);
#endif

#endif // MOTION_H
