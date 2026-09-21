/* Input header file */
#ifndef INPUT_H
#define INPUT_H

// Pins for the turning knob
#define ENCODER_CLK_PIN 18
#define ENCODER_DT_PIN  19
// Pin for the knob button
#define ENCODER_SW_PIN  5

// Pages that can be shown on the screen
typedef enum {
    DISPLAY_TEMPERATURE,
    DISPLAY_HUMIDITY,
    DISPLAY_LIGHT,
    DISPLAY_MOTION
} DisplayMode;

// Find the next page
DisplayMode nextDisplayMode(DisplayMode current);
// Find the previous page
DisplayMode previousDisplayMode(DisplayMode current);

#ifndef TEST_NATIVE
// Setup the knob hardware
void initInput(void);
// Main loop for checking the knob
void InputTask(void* pvParameters);
#endif

#endif // INPUT_H
