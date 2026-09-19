#include "display.h"
#include "rtos_objects.h"
#include "sensors.h"
#include "font5x7.h"

#include "driver/i2c_master.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "display";

#define OLED_SDA GPIO_NUM_21
#define OLED_SCL GPIO_NUM_22
#define OLED_ADDR 0x3C
#define OLED_WIDTH 128
#define OLED_PAGES 8

static i2c_master_bus_handle_t s_bus = nullptr;
static i2c_master_dev_handle_t s_dev = nullptr;
static uint8_t s_frame[OLED_PAGES][OLED_WIDTH];

// DisplayMode is owned by DisplayTask; InputTask requests a change via
// this mutex-protected variable rather than writing to the OLED directly,
// which is what "DisplayTask owns the OLED" means in practice.
static DisplayMode s_requestedMode = DisplayMode::TEMPERATURE;
static SemaphoreHandle_t s_modeMutex = nullptr;

DisplayMode nextDisplayMode(DisplayMode current) {
    switch (current) {
        case DisplayMode::TEMPERATURE: return DisplayMode::HUMIDITY;
        case DisplayMode::HUMIDITY:    return DisplayMode::LIGHT;
        case DisplayMode::LIGHT:       return DisplayMode::MOTION;
        case DisplayMode::MOTION:      return DisplayMode::TEMPERATURE;
    }
    return DisplayMode::TEMPERATURE;
}

DisplayMode previousDisplayMode(DisplayMode current) {
    switch (current) {
        case DisplayMode::TEMPERATURE: return DisplayMode::MOTION;
        case DisplayMode::HUMIDITY:    return DisplayMode::TEMPERATURE;
        case DisplayMode::LIGHT:       return DisplayMode::HUMIDITY;
        case DisplayMode::MOTION:      return DisplayMode::LIGHT;
    }
    return DisplayMode::TEMPERATURE;
}

void displaySetMode(DisplayMode mode) {
    if (s_modeMutex && xSemaphoreTake(s_modeMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        s_requestedMode = mode;
        xSemaphoreGive(s_modeMutex);
    }
}

static DisplayMode getRequestedMode() {
    DisplayMode m = DisplayMode::TEMPERATURE;
    if (s_modeMutex && xSemaphoreTake(s_modeMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        m = s_requestedMode;
        xSemaphoreGive(s_modeMutex);
    }
    return m;
}

static void ssd1306WriteCmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd}; // control byte 0x00 = command
    i2c_master_transmit(s_dev, buf, sizeof(buf), 100);
}

static void ssd1306Flush() {
    for (int page = 0; page < OLED_PAGES; page++) {
        ssd1306WriteCmd(0xB0 + page);
        ssd1306WriteCmd(0x00);
        ssd1306WriteCmd(0x10);
        uint8_t buf[OLED_WIDTH + 1];
        buf[0] = 0x40; // control byte 0x40 = data stream
        memcpy(&buf[1], s_frame[page], OLED_WIDTH);
        i2c_master_transmit(s_dev, buf, sizeof(buf), 100);
    }
}

static void frameClear() {
    memset(s_frame, 0, sizeof(s_frame));
}

// Draws text at (col, page) using the 5x7 font, 1px spacing between glyphs.
static void drawText(int col, int page, const char *text) {
    if (page < 0 || page >= OLED_PAGES) return;
    for (const char *p = text; *p != '\0' && col < OLED_WIDTH - 5; p++, col += 6) {
        int idx = font5x7_index(*p);
        for (int c = 0; c < 5; c++) {
            s_frame[page][col + c] = FONT5x7[idx][c];
        }
    }
}

void displayInit() {
    s_modeMutex = xSemaphoreCreateMutex();

    i2c_master_bus_config_t busCfg = {};
    busCfg.i2c_port = I2C_NUM_0;
    busCfg.sda_io_num = OLED_SDA;
    busCfg.scl_io_num = OLED_SCL;
    busCfg.clk_source = I2C_CLK_SRC_DEFAULT;
    busCfg.glitch_ignore_cnt = 7;
    i2c_new_master_bus(&busCfg, &s_bus);

    i2c_device_config_t devCfg = {};
    devCfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    devCfg.device_address = OLED_ADDR;
    devCfg.scl_speed_hz = 400000;
    i2c_master_bus_add_device(s_bus, &devCfg, &s_dev);

    static const uint8_t initSeq[] = {
        0xAE,       // display off
        0xD5, 0x80, // clock div
        0xA8, 0x3F, // multiplex 64
        0xD3, 0x00, // display offset
        0x40,       // start line 0
        0x8D, 0x14, // charge pump enable
        0x20, 0x00, // horizontal addressing mode
        0xA1,       // segment remap
        0xC8,       // COM scan direction
        0xDA, 0x12, // COM pins
        0x81, 0xCF, // contrast
        0xD9, 0xF1, // precharge
        0xDB, 0x40, // VCOMH deselect
        0xA4,       // resume to RAM content
        0xA6,       // normal (not inverted) display
        0xAF        // display ON
    };
    for (uint8_t cmd : initSeq) {
        ssd1306WriteCmd(cmd);
    }
    frameClear();
    ssd1306Flush();
    ESP_LOGI(TAG, "SSD1306 initialized");
}

static void renderMode(DisplayMode mode, const SensorData &data, bool systemActive) {
    frameClear();
    drawText(0, 0, "ROOM MONITOR");

    if (!systemActive) {
        drawText(0, 3, "INACTIVE");
        ssd1306Flush();
        return;
    }

    char line[24];
    switch (mode) {
        case DisplayMode::TEMPERATURE:
            drawText(0, 2, "TEMPERATURE");
            snprintf(line, sizeof(line), "%.1f C", data.temperature);
            drawText(0, 4, line);
            break;
        case DisplayMode::HUMIDITY:
            drawText(0, 2, "HUMIDITY");
            snprintf(line, sizeof(line), "%.1f %%", data.humidity);
            drawText(0, 4, line);
            break;
        case DisplayMode::LIGHT:
            drawText(0, 2, "LIGHT");
            snprintf(line, sizeof(line), "%d %%", data.lightLevel);
            drawText(0, 4, line);
            break;
        case DisplayMode::MOTION:
            drawText(0, 2, "MOTION");
            drawText(0, 4, data.motionDetected ? "DETECTED" : "NONE");
            break;
    }
    ssd1306Flush();
}

// DisplayTask is the ONLY task that touches the OLED/I2C bus, avoiding the
// need for a display-specific mutex: ownership by a single task is the
// simplest form of mutual exclusion.
void DisplayTask(void *pvParameters) {
    (void)pvParameters;
    SensorData latest = {25.0f, 50.0f, 50, false};

    for (;;) {
        SensorData incoming;
        // Block up to 500ms waiting for new data so we still redraw
        // promptly on a mode change even if a fresh sample hasn't arrived.
        if (xQueueReceive(sensorQueueDisplay, &incoming, pdMS_TO_TICKS(500)) == pdTRUE) {
            latest = incoming;
        }

        EventBits_t bits = xEventGroupGetBits(systemEventGroup);
        bool active = (bits & EVENT_ACTIVE) != 0;

        renderMode(getRequestedMode(), latest, active);
    }
}
