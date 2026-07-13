#ifndef INC_SAND_SIM_H_
#define INC_SAND_SIM_H_

#include <stdbool.h>
#include <stdint.h>

#define SAND_SIM_WIDTH   16U
#define SAND_SIM_HEIGHT  16U
#define SAND_SIM_BYTES   ((SAND_SIM_WIDTH * SAND_SIM_HEIGHT) / 8U)

typedef bool (*SandSimBoundCallback)(int8_t x, int8_t y);
typedef void (*SandSimSetCallback)(int8_t x, int8_t y, bool enabled);

typedef struct {
    uint8_t buffer[SAND_SIM_BYTES];
    SandSimBoundCallback bound_cb;
    SandSimSetCallback set_cb;
} SandSim;

void SandSim_Init(SandSim *sim,
                  SandSimBoundCallback bound_cb,
                  SandSimSetCallback set_cb);
void SandSim_Clear(SandSim *sim);
bool SandSim_Get(const SandSim *sim, uint8_t x, uint8_t y);
bool SandSim_GetIndex(const SandSim *sim, uint16_t index);
void SandSim_Set(SandSim *sim, uint8_t x, uint8_t y, bool enabled);
void SandSim_SetIndex(SandSim *sim, uint16_t index, bool enabled);
void SandSim_Step(SandSim *sim, int16_t angle_deg);

#endif /* INC_SAND_SIM_H_ */

