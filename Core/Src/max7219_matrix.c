#include "max7219_matrix.h"

#include <string.h>

#include "sand_clock_config.h"

#define MAX7219_REG_NOOP         0x00U
#define MAX7219_REG_DIGIT0       0x01U
#define MAX7219_REG_DECODE_MODE  0x09U
#define MAX7219_REG_INTENSITY    0x0AU
#define MAX7219_REG_SCAN_LIMIT   0x0BU
#define MAX7219_REG_SHUTDOWN     0x0CU
#define MAX7219_REG_DISPLAY_TEST 0x0FU

static uint8_t display_buffer[MAX7219_MATRIX_MODULES * MAX7219_MATRIX_HEIGHT];

static void max7219_write_pin(GPIO_TypeDef *port, uint16_t pin, bool state) {
    HAL_GPIO_WritePin(port, pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void max7219_clock_pulse(void) {
    max7219_write_pin(MAX7219_CLK_GPIO_Port, MAX7219_CLK_Pin, true);
    max7219_write_pin(MAX7219_CLK_GPIO_Port, MAX7219_CLK_Pin, false);
}

static void max7219_shift_byte(uint8_t value) {
    for (uint8_t mask = 0x80U; mask != 0U; mask >>= 1U) {
        max7219_write_pin(MAX7219_DIN_GPIO_Port,
                          MAX7219_DIN_Pin,
                          (value & mask) != 0U);
        max7219_clock_pulse();
    }
}

static void max7219_send_pair(uint8_t address, uint8_t value) {
    max7219_shift_byte(address);
    max7219_shift_byte(value);
}

static void max7219_begin_transfer(void) {
    max7219_write_pin(MAX7219_CS_GPIO_Port, MAX7219_CS_Pin, false);
}

static void max7219_end_transfer(void) {
    max7219_write_pin(MAX7219_CS_GPIO_Port, MAX7219_CS_Pin, true);
}

static void max7219_send_command_all(uint8_t address, uint8_t value) {
    max7219_begin_transfer();
    for (uint8_t module = 0U; module < MAX7219_MATRIX_MODULES; module++) {
        max7219_send_pair(address, value);
    }
    max7219_end_transfer();
}

static int16_t max7219_position(int8_t x, int8_t y, uint8_t *bit) {
#if SAND_CLOCK_DISPLAY_MIRROR_X
    x = (int8_t)(MAX7219_MATRIX_WIDTH - 1U - x);
#endif
#if SAND_CLOCK_DISPLAY_MIRROR_Y
    y = (int8_t)(MAX7219_MATRIX_HEIGHT - 1U - y);
#endif

    if (x < 0 || y < 0 ||
        x >= (int8_t)MAX7219_MATRIX_WIDTH ||
        y >= (int8_t)MAX7219_MATRIX_HEIGHT) {
        return -1;
    }

    *bit = (uint8_t)x & 7U;

    /*
     * Buffer layout mirrors GyverMAX7219<2, 1> default addressing:
     * even bytes are the second 8x8 module, odd bytes are the first one.
     */
    return (int16_t)((1U - ((uint8_t)x >> 3U)) +
                     ((uint8_t)y * MAX7219_MATRIX_MODULES));
}

void MAX7219_MatrixInit(void) {
    max7219_write_pin(MAX7219_CS_GPIO_Port, MAX7219_CS_Pin, true);
    max7219_write_pin(MAX7219_CLK_GPIO_Port, MAX7219_CLK_Pin, false);
    max7219_write_pin(MAX7219_DIN_GPIO_Port, MAX7219_DIN_Pin, false);

    HAL_Delay(10U);

    max7219_send_command_all(MAX7219_REG_DISPLAY_TEST, 0x00U);
    max7219_send_command_all(MAX7219_REG_DECODE_MODE, 0x00U);
    max7219_send_command_all(MAX7219_REG_SCAN_LIMIT, 0x07U);
    max7219_send_command_all(MAX7219_REG_INTENSITY,
                             SAND_CLOCK_DEFAULT_BRIGHTNESS & 0x0FU);
    max7219_send_command_all(MAX7219_REG_SHUTDOWN, 0x01U);

    MAX7219_MatrixClear();
    MAX7219_MatrixUpdate();
}

void MAX7219_MatrixClear(void) {
    memset(display_buffer, 0, sizeof(display_buffer));
}

void MAX7219_MatrixFill(bool enabled) {
    memset(display_buffer, enabled ? 0xFF : 0x00, sizeof(display_buffer));
}

void MAX7219_MatrixUpdate(void) {
    uint16_t index = 0U;

    for (uint8_t row = 0U; row < MAX7219_MATRIX_HEIGHT; row++) {
        max7219_begin_transfer();
        for (uint8_t module = 0U; module < MAX7219_MATRIX_MODULES; module++) {
            (void)module;
            max7219_send_pair((uint8_t)(MAX7219_REG_DIGIT0 +
                                        (MAX7219_MATRIX_HEIGHT - 1U - row)),
                              display_buffer[index++]);
        }
        max7219_end_transfer();
    }
}

void MAX7219_MatrixSetBrightness(uint8_t brightness) {
    if (brightness > 15U) {
        brightness = 15U;
    }
    max7219_send_command_all(MAX7219_REG_INTENSITY, brightness);
}

void MAX7219_MatrixDot(int8_t x, int8_t y, bool enabled) {
    uint8_t bit = 0U;
    int16_t pos = max7219_position(x, y, &bit);

    if (pos < 0) {
        return;
    }

    if (enabled) {
        display_buffer[pos] |= (uint8_t)(1U << bit);
    } else {
        display_buffer[pos] &= (uint8_t)~(1U << bit);
    }
}

