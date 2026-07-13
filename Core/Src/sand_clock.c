#include "sand_clock.h"

#include <stdbool.h>
#include <stdint.h>

#include "button_input.h"
#include "buzzer.h"
#include "display_text.h"
#include "imu_orientation.h"
#include "max7219_matrix.h"
#include "sand_clock_config.h"
#include "sand_sim.h"
#include "settings_storage.h"

typedef struct {
    int16_t seconds;
    uint8_t brightness;
} SandClockSettings;

static SandClockSettings settings = {
    SAND_CLOCK_DEFAULT_SECONDS,
    SAND_CLOCK_DEFAULT_BRIGHTNESS,
};

static SandSim sand;
static ButtonInput button_up;
static ButtonInput button_down;
static ButtonInput button_key;

static uint32_t fall_interval_ms = 1U;
static uint32_t last_fall_ms = 0U;
static uint32_t popup_until_ms = 0U;
static uint32_t led_last_toggle_ms = 0U;
static uint32_t settings_save_at_ms = 0U;
static bool led_state = false;
static bool sand_empty_flag = true;
static bool button_combo_active = false;
static bool settings_dirty = false;
static bool orientation_ready = false;
static uint8_t orientation_valid_samples = 0U;
static uint32_t orientation_last_poll_ms = 0U;

static bool time_expired(uint32_t now_ms, uint32_t target_ms) {
    return (int32_t)(now_ms - target_ms) >= 0;
}

static void board_led_set(bool enabled) {
#if SAND_CLOCK_LED_ACTIVE_LOW
    HAL_GPIO_WritePin(ULED_GPIO_Port,
                      ULED_Pin,
                      enabled ? GPIO_PIN_RESET : GPIO_PIN_SET);
#else
    HAL_GPIO_WritePin(ULED_GPIO_Port,
                      ULED_Pin,
                      enabled ? GPIO_PIN_SET : GPIO_PIN_RESET);
#endif
}

static uint32_t calculate_fall_interval(void) {
    if (settings.seconds <= 0) {
        return 1U;
    }

    uint32_t interval =
        ((uint32_t)settings.seconds * 1000UL) /
        (uint32_t)SAND_CLOCK_PARTICLE_COUNT;
    return interval == 0U ? 1U : interval;
}

static void schedule_settings_save(void) {
    settings_dirty = true;
    settings_save_at_ms =
        HAL_GetTick() + SAND_CLOCK_SETTINGS_SAVE_DELAY_MS;
}

static void update_settings_save(uint32_t now_ms) {
    if (!settings_dirty || !time_expired(now_ms, settings_save_at_ms)) {
        return;
    }

    SettingsStorageData stored = {
        settings.seconds,
        settings.brightness,
    };

    if (SettingsStorage_Save(&stored)) {
        settings_dirty = false;
    } else {
        settings_save_at_ms =
            now_ms + SAND_CLOCK_SETTINGS_SAVE_DELAY_MS;
    }
}

static bool sand_bound(int8_t x, int8_t y) {
    if (y >= 8 && x < 8) {
        return false;
    }
    if (y < 8 && x >= 8) {
        return false;
    }

    if (ImuOrientation_GetDir() > 0) {
        if (x == 8 && y == 8) {
            return false;
        }
    } else {
        if (x == 7 && y == 7) {
            return false;
        }
    }

    return x >= 0 && y >= 0 &&
           x < (int8_t)SAND_SIM_WIDTH &&
           y < (int8_t)SAND_SIM_HEIGHT;
}

static void sand_set_display(int8_t x, int8_t y, bool enabled) {
    if (y >= 8) {
        y = (int8_t)(y - 8);
    }
    MAX7219_MatrixDot(x, y, enabled);
}

static void draw_sand(void) {
    MAX7219_MatrixClear();
    for (uint8_t y = 0U; y < SAND_SIM_HEIGHT; y++) {
        for (uint8_t x = 0U; x < SAND_SIM_WIDTH; x++) {
            if (SandSim_Get(&sand, x, y)) {
                sand_set_display((int8_t)x, (int8_t)y, true);
            }
        }
    }
    MAX7219_MatrixUpdate();
}

static void reset_sand(void) {
    SandSim_Clear(&sand);
    for (uint8_t n = 0U; n < SAND_CLOCK_PARTICLE_COUNT; n++) {
        SandSim_Set(&sand, (uint8_t)(n % 8U), (uint8_t)(n / 8U), true);
    }
    sand_empty_flag = true;
    last_fall_ms = HAL_GetTick();
    draw_sand();
}

static void show_time_popup(void) {
    MAX7219_MatrixClear();
    DisplayText_PrintTime((uint16_t)settings.seconds);
    MAX7219_MatrixUpdate();
    popup_until_ms = HAL_GetTick() + SAND_CLOCK_TIME_POPUP_MS;
}

static void change_time(int16_t delta) {
    int32_t next = (int32_t)settings.seconds + delta;

    if (next < 0) {
        next = 0;
    }
    if (next > SAND_CLOCK_MAX_SECONDS) {
        next = SAND_CLOCK_MAX_SECONDS;
    }

    settings.seconds = (int16_t)next;
    fall_interval_ms = calculate_fall_interval();
    last_fall_ms = HAL_GetTick();
    schedule_settings_save();
    show_time_popup();
}

static void change_brightness(int8_t delta) {
    int16_t next = (int16_t)settings.brightness + delta;

    if (next > 15) {
        next = 0;
    }
    if (next < 0) {
        next = 15;
    }

    settings.brightness = (uint8_t)next;
    MAX7219_MatrixSetBrightness(settings.brightness);
    schedule_settings_save();
}

static bool external_button_presses_are_close(void) {
    int32_t difference =
        (int32_t)(button_up.pressed_at_ms - button_down.pressed_at_ms);

    if (difference < 0) {
        difference = -difference;
    }

    return (uint32_t)difference <= SAND_CLOCK_BUTTON_COMBO_MS;
}

static void update_buttons(uint32_t now_ms) {
    ButtonInput_Update(&button_up, now_ms);
    ButtonInput_Update(&button_down, now_ms);
    ButtonInput_Update(&button_key, now_ms);

    bool up_pressed = ButtonInput_IsPressed(&button_up);
    bool down_pressed = ButtonInput_IsPressed(&button_down);

    if (button_combo_active) {
        ButtonInput_ClearEvents(&button_up);
        ButtonInput_ClearEvents(&button_down);

        if (!up_pressed && !down_pressed) {
            button_combo_active = false;
        }
        return;
    }

    if (up_pressed && down_pressed &&
        external_button_presses_are_close()) {
        button_combo_active = true;
        ButtonInput_ClearEvents(&button_up);
        ButtonInput_ClearEvents(&button_down);
        change_brightness(1);
        return;
    }

    if (ButtonInput_ConsumeClick(&button_key)) {
        reset_sand();
    }
    if (ButtonInput_ConsumeHold(&button_key) ||
        ButtonInput_ConsumeStep(&button_key)) {
        change_brightness(1);
    }

    if (ButtonInput_ConsumeClick(&button_up)) {
        change_time(1);
    }
    if (ButtonInput_ConsumeHold(&button_up) ||
        ButtonInput_ConsumeStep(&button_up)) {
        change_time(10);
    }

    if (ButtonInput_ConsumeClick(&button_down)) {
        change_time(-1);
    }
    if (ButtonInput_ConsumeHold(&button_down) ||
        ButtonInput_ConsumeStep(&button_down)) {
        change_time(-10);
    }
}

static void update_led(uint32_t now_ms) {
    uint32_t period_ms = ImuOrientation_IsOnline() ? 1000U : 150U;

    if ((uint32_t)(now_ms - led_last_toggle_ms) >= period_ms) {
        led_last_toggle_ms = now_ms;
        led_state = !led_state;
        board_led_set(led_state);
    }
}

static void update_sand_motion(void) {
    if (!orientation_ready) {
        uint32_t now_ms = HAL_GetTick();

        if ((uint32_t)(now_ms - orientation_last_poll_ms) < 20U) {
            return;
        }
        orientation_last_poll_ms = now_ms;

        if (!ImuOrientation_Update(0U)) {
            orientation_valid_samples = 0U;
            return;
        }

        orientation_valid_samples++;
        if (orientation_valid_samples < 3U) {
            return;
        }

        orientation_ready = true;
        reset_sand();
        return;
    }

    uint16_t update_period_ms = (uint16_t)(255U - ImuOrientation_GetMag());

    if (update_period_ms < 15U) {
        update_period_ms = 15U;
    }
    if (update_period_ms > 90U) {
        update_period_ms = 90U;
    }

    if (!ImuOrientation_Update(update_period_ms)) {
        return;
    }

    if (Buzzer_IsActive() && ImuOrientation_MotionDetected()) {
        Buzzer_Stop();
    }

    MAX7219_MatrixClear();
    SandSim_Step(&sand, (int16_t)(ImuOrientation_GetAngle() + 45));
    MAX7219_MatrixUpdate();
}

static bool source_chamber_is_empty(void) {
    uint8_t first = ImuOrientation_GetDir() > 0 ? 0U : 8U;
    uint8_t end = (uint8_t)(first + 8U);

    for (uint8_t y = first; y < end; y++) {
        for (uint8_t x = first; x < end; x++) {
            if (SandSim_Get(&sand, x, y)) {
                return false;
            }
        }
    }
    return true;
}

static void update_sand_fall(uint32_t now_ms) {
    bool pushed = false;

    if (!orientation_ready) {
        return;
    }

    if ((uint32_t)(now_ms - last_fall_ms) < fall_interval_ms) {
        return;
    }
    last_fall_ms = now_ms;

    if (ImuOrientation_GetDir() > 0) {
        if (SandSim_Get(&sand, 7U, 7U) &&
            !SandSim_Get(&sand, 8U, 8U)) {
            SandSim_Set(&sand, 7U, 7U, false);
            SandSim_Set(&sand, 8U, 8U, true);
            sand_set_display(7, 7, false);
            sand_set_display(8, 8, true);
            pushed = true;
        }
    } else {
        if (SandSim_Get(&sand, 8U, 8U) &&
            !SandSim_Get(&sand, 7U, 7U)) {
            SandSim_Set(&sand, 8U, 8U, false);
            SandSim_Set(&sand, 7U, 7U, true);
            sand_set_display(8, 8, false);
            sand_set_display(7, 7, true);
            pushed = true;
        }
    }

    if (pushed) {
        sand_empty_flag = false;
        MAX7219_MatrixUpdate();
    } else if (!sand_empty_flag && source_chamber_is_empty()) {
        sand_empty_flag = true;
        Buzzer_PlayAlarm();
    }
}

void SandClock_Init(void) {
    SettingsStorageData stored;

    board_led_set(false);
    Buzzer_Init();

    if (SettingsStorage_Load(&stored)) {
        settings.seconds = stored.seconds;
        settings.brightness = stored.brightness;
    }

    MAX7219_MatrixInit();
    MAX7219_MatrixSetBrightness(settings.brightness);

    ButtonInput_Init(&button_up,
                     EXT_BTN1_GPIO_Port,
                     EXT_BTN1_Pin,
                     true);
    ButtonInput_Init(&button_down,
                     EXT_BTN2_GPIO_Port,
                     EXT_BTN2_Pin,
                     true);
    ButtonInput_Init(&button_key,
                     UKEY_GPIO_Port,
                     UKEY_Pin,
                     SAND_CLOCK_UKEY_ACTIVE_LOW != 0);

    SandSim_Init(&sand, sand_bound, sand_set_display);
    orientation_ready = false;
    orientation_valid_samples = 0U;
    orientation_last_poll_ms = HAL_GetTick();

    /*
     * On external 5 V power the MPU module becomes operational noticeably
     * later than the STM32. Give its regulator and oscillator time to settle
     * before the first WHO_AM_I transaction.
     */
    HAL_Delay(SAND_CLOCK_MPU_POWERUP_DELAY_MS);
    (void)ImuOrientation_Init();
    orientation_last_poll_ms = HAL_GetTick();

    fall_interval_ms = calculate_fall_interval();
    last_fall_ms = HAL_GetTick();

    MAX7219_MatrixClear();
    MAX7219_MatrixUpdate();
}

void SandClock_Tick(void) {
    uint32_t now_ms = HAL_GetTick();

    update_buttons(now_ms);
    Buzzer_Update(now_ms);
    update_led(now_ms);
    update_settings_save(now_ms);

    if (popup_until_ms != 0U) {
        if (!time_expired(now_ms, popup_until_ms)) {
            return;
        }
        popup_until_ms = 0U;
        draw_sand();
    }

    update_sand_motion();
    update_sand_fall(now_ms);
}
