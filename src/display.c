/* Handles showing information on the OLED screen */
#include "display.h"
#include "rtos_objects.h"

#ifndef TEST_NATIVE
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_ssd1306.h"

#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define OLED_PAGES  (OLED_HEIGHT / 8)

// Connection bus to the screen
static i2c_master_bus_handle_t s_i2c_bus_handle = NULL;
// Tool to send data to the screen
static esp_lcd_panel_io_handle_t s_io_handle = NULL;
// The screen panel itself
static esp_lcd_panel_handle_t s_panel_handle = NULL;

// Letters and numbers to draw on the screen
static const uint8_t s_font5x8[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, 
    {0x00, 0x00, 0x5F, 0x00, 0x00}, 
    {0x00, 0x07, 0x00, 0x07, 0x00}, 
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, 
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, 
    {0x23, 0x13, 0x08, 0x64, 0x62}, 
    {0x36, 0x49, 0x55, 0x22, 0x50}, 
    {0x00, 0x05, 0x03, 0x00, 0x00}, 
    {0x00, 0x1C, 0x22, 0x41, 0x00}, 
    {0x00, 0x41, 0x22, 0x1C, 0x00}, 
    {0x14, 0x08, 0x3E, 0x08, 0x14}, 
    {0x08, 0x08, 0x3E, 0x08, 0x08}, 
    {0x00, 0x50, 0x30, 0x00, 0x00}, 
    {0x08, 0x08, 0x08, 0x08, 0x08}, 
    {0x00, 0x60, 0x60, 0x00, 0x00}, 
    {0x20, 0x10, 0x08, 0x04, 0x02}, 
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, 
    {0x00, 0x42, 0x7F, 0x40, 0x00}, 
    {0x42, 0x61, 0x51, 0x49, 0x46}, 
    {0x21, 0x41, 0x45, 0x4B, 0x31}, 
    {0x18, 0x14, 0x12, 0x7F, 0x10}, 
    {0x27, 0x45, 0x45, 0x45, 0x39}, 
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, 
    {0x01, 0x71, 0x09, 0x05, 0x03}, 
    {0x36, 0x49, 0x49, 0x49, 0x36}, 
    {0x06, 0x49, 0x49, 0x29, 0x1E}, 
    {0x00, 0x36, 0x36, 0x00, 0x00}, 
    {0x00, 0x56, 0x36, 0x00, 0x00}, 
    {0x08, 0x14, 0x22, 0x41, 0x00}, 
    {0x14, 0x14, 0x14, 0x14, 0x14}, 
    {0x00, 0x41, 0x22, 0x14, 0x08}, 
    {0x02, 0x01, 0x51, 0x09, 0x06}, 
    {0x32, 0x49, 0x79, 0x41, 0x3E}, 
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, 
    {0x7F, 0x49, 0x49, 0x49, 0x36}, 
    {0x3E, 0x41, 0x41, 0x41, 0x22}, 
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, 
    {0x7F, 0x49, 0x49, 0x49, 0x41}, 
    {0x7F, 0x09, 0x09, 0x09, 0x01}, 
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, 
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, 
    {0x00, 0x41, 0x7F, 0x41, 0x00}, 
    {0x20, 0x40, 0x41, 0x3F, 0x01}, 
    {0x7F, 0x08, 0x14, 0x22, 0x41}, 
    {0x7F, 0x40, 0x40, 0x40, 0x40}, 
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, 
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, 
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, 
    {0x7F, 0x09, 0x09, 0x09, 0x06}, 
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, 
    {0x7F, 0x09, 0x19, 0x29, 0x46}, 
    {0x46, 0x49, 0x49, 0x49, 0x31}, 
    {0x01, 0x01, 0x7F, 0x01, 0x01}, 
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, 
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, 
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, 
    {0x63, 0x14, 0x08, 0x14, 0x63}, 
    {0x07, 0x08, 0x70, 0x08, 0x07}, 
    {0x61, 0x51, 0x49, 0x45, 0x43}, 
    {0x00, 0x7F, 0x41, 0x41, 0x00}, 
    {0x02, 0x04, 0x08, 0x10, 0x20}, 
    {0x00, 0x41, 0x41, 0x7F, 0x00}, 
    {0x04, 0x02, 0x01, 0x02, 0x04}, 
    {0x40, 0x40, 0x40, 0x40, 0x40}  
};

// Memory that holds the picture before sending it to the screen
static uint8_t s_framebuffer[OLED_PAGES][OLED_WIDTH];

/* Wipe the memory clean */
static void ssd1306_clear_buffer(void) {
    memset(s_framebuffer, 0, sizeof(s_framebuffer));
}

/* Draw one letter on the screen memory */
static void ssd1306_draw_char(int page, int col, char c) {
    if (page < 0 || page >= OLED_PAGES || col < 0 || col > OLED_WIDTH - 6) {
        return;
    }
    if (c >= 'a' && c <= 'z') {
        c = c - 'a' + 'A';
    }
    if (c < 32 || c > 95) {
        c = ' ';
    }
    int index = c - 32;
    for (int i = 0; i < 5; ++i) {
        s_framebuffer[page][col + i] = s_font5x8[index][i];
    }
    s_framebuffer[page][col + 5] = 0x00;
}

/* Draw a word or sentence on the screen memory */
static void ssd1306_draw_string(int page, int col, const char* str) {
    int curCol = col;
    while (*str && curCol <= OLED_WIDTH - 6) {
        ssd1306_draw_char(page, curCol, *str++);
        curCol += 6;
    }
}

/* Send the picture memory to the real screen */
static void ssd1306_update_screen(void) {
    if (s_panel_handle == NULL) {
        return;
    }
    esp_lcd_panel_draw_bitmap(s_panel_handle, 0, 0, OLED_WIDTH, OLED_HEIGHT, s_framebuffer);
}

/* Setup the screen so we can talk to it */
void initDisplay(void) {
    i2c_master_bus_config_t bus_config = {};
    bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_config.i2c_port = I2C_NUM_0;
    bus_config.scl_io_num = (gpio_num_t)I2C_MASTER_SCL_IO;
    bus_config.sda_io_num = (gpio_num_t)I2C_MASTER_SDA_IO;
    bus_config.glitch_ignore_cnt = 7;
    bus_config.flags.enable_internal_pullup = 1;
    esp_err_t err = i2c_new_master_bus(&bus_config, &s_i2c_bus_handle);
    if (err != ESP_OK) {
        printf("[Display] Error creating I2C master bus: %d\n", err);
        return;
    }

    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = SSD1306_I2C_ADDRESS,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
        .control_phase_bytes = 1,
        .dc_bit_offset = 6,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .on_color_trans_done = NULL,
        .user_ctx = NULL,
        .flags = {
            .dc_low_on_data = 0,
            .disable_control_phase = 0,
        },
    };
    err = esp_lcd_new_panel_io_i2c(s_i2c_bus_handle, &io_config, &s_io_handle);
    if (err != ESP_OK) {
        printf("[Display] Error creating panel IO: %d\n", err);
        return;
    }

    esp_lcd_panel_ssd1306_config_t ssd1306_config = {
        .height = OLED_HEIGHT,
    };
    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.bits_per_pixel = 1;
    panel_config.reset_gpio_num = (gpio_num_t)-1;
    panel_config.vendor_config = &ssd1306_config;
    err = esp_lcd_new_panel_ssd1306(s_io_handle, &panel_config, &s_panel_handle);
    if (err != ESP_OK) {
        printf("[Display] Error creating SSD1306 panel: %d\n", err);
        return;
    }

    esp_lcd_panel_reset(s_panel_handle);
    esp_lcd_panel_init(s_panel_handle);
    esp_lcd_panel_mirror(s_panel_handle, true, true);
    esp_lcd_panel_disp_on_off(s_panel_handle, true);

    ssd1306_clear_buffer();
    ssd1306_draw_string(0, 16, "ROOM MONITOR");
    ssd1306_draw_string(1, 0, "---------------------");
    ssd1306_draw_string(3, 4, "Temperature");
    ssd1306_draw_string(5, 8, "  25.4 C");
    ssd1306_update_screen();
    printf("[Display] SSD1306 initialized successfully.\n");
}

/* Task that runs forever to show new data on the screen */
void DisplayTask(void* pvParameters) {
    (void)pvParameters;
    SensorData sensorData = { 25.4f, 60.0f, 50, false };
    DisplayMode currentMode = DISPLAY_TEMPERATURE;
    bool wasActive = true;

    char line1[24];
    char line2[24];

    for (;;) {
        // Check if the system is awake
        EventBits_t bits = xEventGroupGetBits(g_system_event_group);
        bool isActive = (bits & EVENT_ACTIVE) != 0;

        // If the system is awake, update the screen
        if (isActive) {
            if (!wasActive) {
                if (s_panel_handle != NULL) {
                    esp_lcd_panel_disp_on_off(s_panel_handle, true);
                }
                wasActive = true;
            }

            // Get the newest sensor data without removing it from the mailbox
            xQueuePeek(g_sensor_queue, &sensorData, 0);

            ssd1306_clear_buffer();

            ssd1306_draw_string(0, 16, "ROOM MONITOR");
            ssd1306_draw_string(1, 0, "---------------------");

            switch (currentMode) {
                case DISPLAY_TEMPERATURE:
                    snprintf(line1, sizeof(line1), "Temperature");
                    snprintf(line2, sizeof(line2), "  %.1f C", sensorData.temperature);
                    break;
                case DISPLAY_HUMIDITY:
                    snprintf(line1, sizeof(line1), "Humidity");
                    snprintf(line2, sizeof(line2), "  %.1f %%", sensorData.humidity);
                    break;
                case DISPLAY_LIGHT:
                    snprintf(line1, sizeof(line1), "Ambient Light");
                    snprintf(line2, sizeof(line2), "  %d %%", sensorData.lightLevel);
                    break;
                case DISPLAY_MOTION:
                    snprintf(line1, sizeof(line1), "Motion Sensor");
                    snprintf(line2, sizeof(line2), "  %s", sensorData.motionDetected ? "DETECTED" : "CLEAR");
                    break;
                default:
                    snprintf(line1, sizeof(line1), "Temperature");
                    snprintf(line2, sizeof(line2), "  %.1f C", sensorData.temperature);
                    break;
            }

            ssd1306_draw_string(3, 4, line1);
            ssd1306_draw_string(5, 8, line2);

            // Check if the alarm is ringing and show a warning
            if ((bits & EVENT_ALARM) != 0) {
                ssd1306_draw_string(7, 16, "* ALARM ACTIVE *");
            }

            ssd1306_update_screen();

            DisplayMode newMode;
            if (xQueueReceive(g_display_queue, &newMode, pdMS_TO_TICKS(200)) == pdTRUE) {
                currentMode = newMode;
            }
        } else {
            if (wasActive) {
                ssd1306_clear_buffer();
                ssd1306_update_screen();
                if (s_panel_handle != NULL) {
                    esp_lcd_panel_disp_on_off(s_panel_handle, false);
                }
                wasActive = false;
            }
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}
#endif

