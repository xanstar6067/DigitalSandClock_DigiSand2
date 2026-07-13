#include "display_text.h"

#include "max7219_matrix.h"

static const uint8_t font3x5[10][3] = {
    {0x1FU, 0x11U, 0x1FU},
    {0x1FU, 0x00U, 0x00U},
    {0x1DU, 0x15U, 0x17U},
    {0x1FU, 0x15U, 0x15U},
    {0x1FU, 0x04U, 0x1CU},
    {0x17U, 0x15U, 0x1DU},
    {0x17U, 0x15U, 0x1FU},
    {0x1FU, 0x10U, 0x10U},
    {0x1FU, 0x15U, 0x1FU},
    {0x1FU, 0x15U, 0x1DU},
};

void DisplayText_PrintDigit(int8_t x, int8_t y, uint8_t digit) {
    if (digit > 9U) {
        return;
    }

    for (uint8_t col_index = 0U; col_index < 3U; col_index++) {
        uint8_t col = font3x5[digit][col_index];
        for (uint8_t row = 0U; row < 5U; row++) {
            if ((col & (uint8_t)(1U << (4U - row))) != 0U) {
                MAX7219_MatrixDot((int8_t)(x + 2 - col_index),
                                  (int8_t)(y + row),
                                  true);
            }
        }
    }
}

void DisplayText_PrintTwoDigits(int8_t x, int8_t y, uint8_t value) {
    if (value > 99U) {
        value = 99U;
    }

    DisplayText_PrintDigit(x, y, (uint8_t)(value / 10U));
    DisplayText_PrintDigit((int8_t)(x + 4), y, (uint8_t)(value % 10U));
}

void DisplayText_PrintTime(uint16_t seconds) {
    uint8_t minutes = (uint8_t)(seconds / 60U);
    uint8_t secs = (uint8_t)(seconds % 60U);

    if (minutes > 99U) {
        minutes = 99U;
        secs = 99U;
    }

    DisplayText_PrintTwoDigits(0, 1, minutes);
    DisplayText_PrintTwoDigits(8, 1, secs);
}

