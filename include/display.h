/* Display header file */
#ifndef DISPLAY_H
#define DISPLAY_H

#include "input.h"
#include "sensors.h"

// Pins for the screen connection
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
// Speed of the screen connection
#define I2C_MASTER_FREQ_HZ 400000
// Hardware address of the screen
#define SSD1306_I2C_ADDRESS 0x3C

#ifndef TEST_NATIVE
// Setup the screen
void initDisplay(void);
// Main loop for the screen task
void DisplayTask(void* pvParameters);
#endif

#endif // DISPLAY_H
