#ifndef INC_MAX7219_MATRIX_H_
#define INC_MAX7219_MATRIX_H_

#include <stdbool.h>
#include <stdint.h>

#include "main.h"

#define MAX7219_MATRIX_WIDTH    16U
#define MAX7219_MATRIX_HEIGHT   8U
#define MAX7219_MATRIX_MODULES  2U

void MAX7219_MatrixInit(void);
void MAX7219_MatrixClear(void);
void MAX7219_MatrixUpdate(void);
void MAX7219_MatrixSetBrightness(uint8_t brightness);
void MAX7219_MatrixDot(int8_t x, int8_t y, bool enabled);
void MAX7219_MatrixFill(bool enabled);

#endif /* INC_MAX7219_MATRIX_H_ */

