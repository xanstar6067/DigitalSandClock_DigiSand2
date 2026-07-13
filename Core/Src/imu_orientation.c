#include "imu_orientation.h"

#include <math.h>
#include <stdlib.h>

#include "mpu6000.h"
#include "sand_clock_config.h"

typedef struct {
    uint8_t axis;
    int8_t sign;
} AxisMap;

static const AxisMap axis_x = {
    SAND_CLOCK_IMU_X_AXIS,
    SAND_CLOCK_IMU_X_SIGN,
};
static const AxisMap axis_y = {
    SAND_CLOCK_IMU_Y_AXIS,
    SAND_CLOCK_IMU_Y_SIGN,
};
static const AxisMap axis_z = {
    SAND_CLOCK_IMU_Z_AXIS,
    SAND_CLOCK_IMU_Z_SIGN,
};

static int16_t raw_axes[3] = {0, 0, 0};
static int16_t angle_deg = 0;
static uint8_t mag = 0;
static uint32_t last_update_ms = 0;
static bool online = false;

static int16_t mapped_axis(const AxisMap *map) {
    if (map->axis > 2U) {
        return 0;
    }
    return (int16_t)(raw_axes[map->axis] * map->sign);
}

bool ImuOrientation_Init(void) {
    online = (MPU6000_Init() != 0U);
    last_update_ms = 0U;
    return online;
}

bool ImuOrientation_Update(uint16_t period_ms) {
    uint32_t now_ms = HAL_GetTick();
    MPU6000_Data_t data;

    if ((uint32_t)(now_ms - last_update_ms) < period_ms) {
        return false;
    }
    last_update_ms = now_ms;

    if (MPU6000_ReadData(&data) != HAL_OK) {
        online = false;
        return false;
    }

    online = true;
    raw_axes[0] = data.accel_x;
    raw_axes[1] = data.accel_y;
    raw_axes[2] = data.accel_z;

    int32_t z = mapped_axis(&axis_z);
    int32_t mag_raw = (labs(z) * 255L) / 16000L;
    if (mag_raw > 255L) {
        mag_raw = 255L;
    }
    mag = (uint8_t)(255L - mag_raw);

    float x = (float)mapped_axis(&axis_x);
    float y = (float)mapped_axis(&axis_y);
    float deg = atan2f(x, y) * 57.2957795f;
    angle_deg = (int16_t)(deg >= 0.0f ? deg + 0.5f : deg - 0.5f);

    return true;
}

int16_t ImuOrientation_GetAngle(void) {
    return angle_deg;
}

uint8_t ImuOrientation_GetMag(void) {
    return mag;
}

int8_t ImuOrientation_GetDir(void) {
    return (angle_deg > -90 && angle_deg < 90) ? 1 : -1;
}

bool ImuOrientation_IsOnline(void) {
    return online;
}

