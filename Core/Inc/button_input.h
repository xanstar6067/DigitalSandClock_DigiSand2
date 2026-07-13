#ifndef INC_BUTTON_INPUT_H_
#define INC_BUTTON_INPUT_H_

#include <stdbool.h>
#include <stdint.h>

#include "main.h"

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
    bool active_low;
    bool raw_state;
    bool stable_state;
    bool hold_reported;
    uint32_t changed_at_ms;
    uint32_t pressed_at_ms;
    uint32_t last_step_ms;
    bool click_event;
    bool hold_event;
    bool step_event;
} ButtonInput;

void ButtonInput_Init(ButtonInput *button,
                      GPIO_TypeDef *port,
                      uint16_t pin,
                      bool active_low);
void ButtonInput_Update(ButtonInput *button, uint32_t now_ms);
bool ButtonInput_IsPressed(const ButtonInput *button);
bool ButtonInput_ConsumeClick(ButtonInput *button);
bool ButtonInput_ConsumeHold(ButtonInput *button);
bool ButtonInput_ConsumeStep(ButtonInput *button);
void ButtonInput_ClearEvents(ButtonInput *button);

#endif /* INC_BUTTON_INPUT_H_ */

