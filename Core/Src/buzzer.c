#include "buzzer.h"

#include <stdbool.h>

#include "main.h"
#include "sand_clock_config.h"

extern TIM_HandleTypeDef htim1;

static const uint16_t alarm_step_ms[] = {
    120U, 90U,
    120U, 90U,
    120U, 90U,
    320U, 450U,
};

static uint32_t step_ends_at_ms = 0U;
static uint8_t alarm_step = 0U;
static bool alarm_active = false;

static bool time_expired(uint32_t now_ms, uint32_t target_ms) {
    return (int32_t)(now_ms - target_ms) >= 0;
}

static void buzzer_silent(void) {
    (void)HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_1);
    (void)HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
}

static void buzzer_tone(void) {
    uint32_t period =
        SAND_CLOCK_BUZZER_TIMER_HZ / SAND_CLOCK_BUZZER_FREQUENCY_HZ;

    if (period < 2U) {
        period = 2U;
    }

    __HAL_TIM_SET_AUTORELOAD(&htim1, period - 1U);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, period / 2U);
    __HAL_TIM_SET_COUNTER(&htim1, 0U);
    htim1.Instance->EGR = TIM_EGR_UG;

    (void)HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    (void)HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
}

void Buzzer_Init(void) {
    alarm_active = false;
    alarm_step = 0U;
    buzzer_silent();
}

void Buzzer_PlayAlarm(void) {
    alarm_step = 0U;
    alarm_active = true;
    buzzer_tone();
    step_ends_at_ms = HAL_GetTick() + alarm_step_ms[0];
}

void Buzzer_Stop(void) {
    alarm_active = false;
    buzzer_silent();
}

bool Buzzer_IsActive(void) {
    return alarm_active;
}

void Buzzer_Update(uint32_t now_ms) {
    if (!alarm_active || !time_expired(now_ms, step_ends_at_ms)) {
        return;
    }

    alarm_step++;
    if (alarm_step >=
        (uint8_t)(sizeof(alarm_step_ms) / sizeof(alarm_step_ms[0]))) {
        alarm_step = 0U;
    }

    if ((alarm_step & 1U) == 0U) {
        buzzer_tone();
    } else {
        buzzer_silent();
    }

    step_ends_at_ms = now_ms + alarm_step_ms[alarm_step];
}
