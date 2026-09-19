#pragma once
#include <stdbool.h>

void motionInit();
void MotionTask(void *pvParameters);

// Latest debounced motion flag, read by SensorTask when building a sample.
bool motionCurrentlyDetected();
