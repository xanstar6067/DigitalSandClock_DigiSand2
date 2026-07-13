#ifndef INC_IMU_ORIENTATION_H_
#define INC_IMU_ORIENTATION_H_

#include <stdbool.h>
#include <stdint.h>

bool ImuOrientation_Init(void);
bool ImuOrientation_Update(uint16_t period_ms);
int16_t ImuOrientation_GetAngle(void);
uint8_t ImuOrientation_GetMag(void);
int8_t ImuOrientation_GetDir(void);
bool ImuOrientation_IsOnline(void);

#endif /* INC_IMU_ORIENTATION_H_ */

