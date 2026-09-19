#pragma once

enum class DisplayMode {
    TEMPERATURE,
    HUMIDITY,
    LIGHT,
    MOTION
};

// Pure, hardware-independent navigation logic (unit-testable).
DisplayMode nextDisplayMode(DisplayMode current);
DisplayMode previousDisplayMode(DisplayMode current);

void displayInit();          // I2C + SSD1306 init
void DisplayTask(void *pvParameters);

// Called by InputTask to change what DisplayTask should render.
void displaySetMode(DisplayMode mode);
