#include "button_input.h"

#include <string.h>

#include "sand_clock_config.h"

static bool button_read_raw(const ButtonInput *button) {
    GPIO_PinState state = HAL_GPIO_ReadPin(button->port, button->pin);
    bool high = (state == GPIO_PIN_SET);
    return button->active_low ? !high : high;
}

void ButtonInput_Init(ButtonInput *button,
                      GPIO_TypeDef *port,
                      uint16_t pin,
                      bool active_low) {
    GPIO_InitTypeDef gpio = {0};

    memset(button, 0, sizeof(*button));
    button->port = port;
    button->pin = pin;
    button->active_low = active_low;

    gpio.Pin = pin;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = active_low ? GPIO_PULLUP : GPIO_PULLDOWN;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port, &gpio);

    button->raw_state = button_read_raw(button);
    button->stable_state = button->raw_state;
    button->changed_at_ms = HAL_GetTick();
}

void ButtonInput_Update(ButtonInput *button, uint32_t now_ms) {
    bool raw = button_read_raw(button);

    if (raw != button->raw_state) {
        button->raw_state = raw;
        button->changed_at_ms = now_ms;
    }

    if ((uint32_t)(now_ms - button->changed_at_ms) >=
        SAND_CLOCK_BUTTON_DEBOUNCE_MS &&
        button->stable_state != button->raw_state) {
        button->stable_state = button->raw_state;

        if (button->stable_state) {
            button->pressed_at_ms = now_ms;
            button->last_step_ms = now_ms;
            button->hold_reported = false;
        } else {
            if (!button->hold_reported) {
                button->click_event = true;
            }
        }
    }

    if (button->stable_state) {
        uint32_t held_ms = now_ms - button->pressed_at_ms;

        if (!button->hold_reported &&
            held_ms >= SAND_CLOCK_BUTTON_HOLD_MS) {
            button->hold_reported = true;
            button->hold_event = true;
            button->last_step_ms = now_ms;
        }

        if (button->hold_reported &&
            (uint32_t)(now_ms - button->last_step_ms) >=
            SAND_CLOCK_BUTTON_STEP_MS) {
            button->step_event = true;
            button->last_step_ms = now_ms;
        }
    }
}

bool ButtonInput_IsPressed(const ButtonInput *button) {
    return button->stable_state;
}

bool ButtonInput_ConsumeClick(ButtonInput *button) {
    bool event = button->click_event;
    button->click_event = false;
    return event;
}

bool ButtonInput_ConsumeHold(ButtonInput *button) {
    bool event = button->hold_event;
    button->hold_event = false;
    return event;
}

bool ButtonInput_ConsumeStep(ButtonInput *button) {
    bool event = button->step_event;
    button->step_event = false;
    return event;
}

void ButtonInput_ClearEvents(ButtonInput *button) {
    button->click_event = false;
    button->hold_event = false;
    button->step_event = false;
}

