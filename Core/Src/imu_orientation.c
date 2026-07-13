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
static bool have_previous_sample = false;
static bool motion_detected = false;

static uint32_t abs_i16(int16_t value) {
    return value < 0 ? (uint32_t)(-(int32_t)value) : (uint32_t)value;
}

static uint32_t axis_delta(int16_t current, int16_t previous) {
    int32_t difference = (int32_t)current - (int32_t)previous;
    return difference < 0 ? (uint32_t)(-difference) : (uint32_t)difference;
}

static bool acceleration_is_plausible(const MPU6000_Data_t *data) {
    uint32_t magnitude_l1 = abs_i16(data->accel_x) +
                            abs_i16(data->accel_y) +
                            abs_i16(data->accel_z);

    /*
     * At the default +/-2 g range a stationary sensor measures about 16384
     * counts. During a cold power ramp some modules acknowledge I2C before
     * their measurement registers contain a usable vector.
     */
    return magnitude_l1 >= 6000U && magnitude_l1 <= 40000U;
}

static int16_t mapped_axis(const AxisMap *map) {
    if (map->axis > 2U) {
        return 0;
    }
    return (int16_t)(raw_axes[map->axis] * map->sign);
}

bool ImuOrientation_Init(void) {
    bool initialized = (MPU6000_Init() != 0U);
    online = false;
    have_previous_sample = false;
    motion_detected = false;
    last_update_ms = 0U;
    return initialized;
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
        motion_detected = false;
        return false;
    }

    if (!acceleration_is_plausible(&data)) {
        online = false;
        motion_detected = false;
        return false;
    }

    uint32_t acceleration_delta = 0U;
    if (have_previous_sample) {
        acceleration_delta =
            axis_delta(data.accel_x, raw_axes[0]) +
            axis_delta(data.accel_y, raw_axes[1]) +
            axis_delta(data.accel_z, raw_axes[2]);
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
    int16_t new_angle =
        (int16_t)(deg >= 0.0f ? deg + 0.5f : deg - 0.5f);
    int16_t angle_delta = (int16_t)abs(new_angle - angle_deg);
    if (angle_delta > 180) {
        angle_delta = (int16_t)(360 - angle_delta);
    }

    uint32_t gyro_sum = abs_i16(data.gyro_x) +
                        abs_i16(data.gyro_y) +
                        abs_i16(data.gyro_z);
    motion_detected = have_previous_sample &&
        ((uint16_t)angle_delta >= SAND_CLOCK_MOTION_ANGLE_DEG ||
         acceleration_delta >= SAND_CLOCK_MOTION_ACCEL_DELTA ||
         gyro_sum >= SAND_CLOCK_MOTION_GYRO_SUM);
    angle_deg = new_angle;
    have_previous_sample = true;

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

bool ImuOrientation_MotionDetected(void) {
    return motion_detected;
}
