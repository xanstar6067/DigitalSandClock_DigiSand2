#ifndef INC_DISPLAY_TEXT_H_
#define INC_DISPLAY_TEXT_H_

#include <stdint.h>

void DisplayText_PrintDigit(int8_t x, int8_t y, uint8_t digit);
void DisplayText_PrintTime(uint16_t seconds);
void DisplayText_PrintTwoDigits(int8_t x, int8_t y, uint8_t value);

#endif /* INC_DISPLAY_TEXT_H_ */

