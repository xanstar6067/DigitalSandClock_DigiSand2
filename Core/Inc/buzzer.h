#ifndef INC_BUZZER_H_
#define INC_BUZZER_H_

#include <stdbool.h>
#include <stdint.h>

void Buzzer_Init(void);
void Buzzer_PlayAlarm(void);
void Buzzer_Stop(void);
bool Buzzer_IsActive(void);
void Buzzer_Update(uint32_t now_ms);

#endif /* INC_BUZZER_H_ */
