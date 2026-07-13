#include "sand_sim.h"

#include <string.h>

typedef struct {
    int8_t x;
    int8_t y;
} SandDir;

static const SandDir dir_shift[8] = {
    {1, 0},
    {1, 1},
    {0, 1},
    {-1, 1},
    {-1, 0},
    {-1, -1},
    {0, -1},
    {1, -1},
};

static uint16_t sand_rand16(void) {
    static uint16_t seed = 12345U;
    seed = (uint16_t)(((uint32_t)seed * 2053UL) + 13849UL);
    return seed;
}

static uint16_t sand_rand16_max(uint16_t max_value) {
    return (uint16_t)(((uint32_t)max_value * sand_rand16()) >> 16U);
}

static uint16_t sand_rand16_range(uint16_t min_value, uint16_t max_value) {
    return (uint16_t)(sand_rand16_max((uint16_t)(max_value - min_value)) +
                      min_value);
}

static SandDir sand_dir_from_index(int8_t dir) {
    dir = (int8_t)((dir + 8) & 7);
    return dir_shift[(uint8_t)dir];
}

static bool buffer_get_index(const uint8_t *buffer, uint16_t index) {
    return (buffer[index >> 3U] & (uint8_t)(1U << (index & 7U))) != 0U;
}

static void buffer_set_index(uint8_t *buffer, uint16_t index, bool enabled) {
    if (enabled) {
        buffer[index >> 3U] |= (uint8_t)(1U << (index & 7U));
    } else {
        buffer[index >> 3U] &= (uint8_t)~(1U << (index & 7U));
    }
}

void SandSim_Init(SandSim *sim,
                  SandSimBoundCallback bound_cb,
                  SandSimSetCallback set_cb) {
    SandSim_Clear(sim);
    sim->bound_cb = bound_cb;
    sim->set_cb = set_cb;
}

void SandSim_Clear(SandSim *sim) {
    memset(sim->buffer, 0, sizeof(sim->buffer));
}

bool SandSim_Get(const SandSim *sim, uint8_t x, uint8_t y) {
    if (x >= SAND_SIM_WIDTH || y >= SAND_SIM_HEIGHT) {
        return false;
    }
    return SandSim_GetIndex(sim, (uint16_t)(x + (y * SAND_SIM_WIDTH)));
}

bool SandSim_GetIndex(const SandSim *sim, uint16_t index) {
    if (index >= (SAND_SIM_WIDTH * SAND_SIM_HEIGHT)) {
        return false;
    }
    return buffer_get_index(sim->buffer, index);
}

void SandSim_Set(SandSim *sim, uint8_t x, uint8_t y, bool enabled) {
    if (x >= SAND_SIM_WIDTH || y >= SAND_SIM_HEIGHT) {
        return;
    }
    SandSim_SetIndex(sim, (uint16_t)(x + (y * SAND_SIM_WIDTH)), enabled);
}

void SandSim_SetIndex(SandSim *sim, uint16_t index, bool enabled) {
    if (index >= (SAND_SIM_WIDTH * SAND_SIM_HEIGHT)) {
        return;
    }
    buffer_set_index(sim->buffer, index, enabled);
}

void SandSim_Step(SandSim *sim, int16_t angle_deg) {
    uint8_t dir_count = 0U;
    SandDir dirs[3] = {{0, 0}, {0, 0}, {0, 0}};
    uint8_t temp[SAND_SIM_BYTES];

    if (sim->bound_cb == NULL || sim->set_cb == NULL) {
        return;
    }

    while (angle_deg < 0) {
        angle_deg = (int16_t)(angle_deg + 360);
    }
    while (angle_deg >= 360) {
        angle_deg = (int16_t)(angle_deg - 360);
    }

    uint8_t angle =
        (uint8_t)((((uint16_t)angle_deg * 2U) + 22U) / 45U);
    angle &= 15U;
    uint8_t quadrant = angle >> 2U;

    SandDir cur = sand_dir_from_index((int8_t)(quadrant * 2U));
    SandDir next = sand_dir_from_index((int8_t)((quadrant + 1U) * 2U));
    SandDir diag = sand_dir_from_index((int8_t)(quadrant * 2U + 1U));

    if ((angle & 3U) == 0U) {
        dir_count = 1U;
        dirs[0] = cur;
    } else if ((angle & 1U) == 0U) {
        dir_count = 3U;
        dirs[0] = diag;
        if (sand_rand16() > 0x7FFFU) {
            dirs[1] = cur;
            dirs[2] = next;
        } else {
            dirs[1] = next;
            dirs[2] = cur;
        }
    } else {
        dir_count = 3U;
        switch (angle) {
        case 1:
        case 5:
        case 9:
        case 13:
            dirs[0] = cur;
            dirs[1] = diag;
            dirs[2] = next;
            break;
        default:
            dirs[0] = next;
            dirs[1] = diag;
            dirs[2] = cur;
            break;
        }
    }

    memcpy(temp, sim->buffer, sizeof(temp));

    uint8_t stride = (uint8_t)sand_rand16_range(3U, 20U);
    for (uint8_t start = 0U; start < stride; start++) {
        for (uint16_t i = start;
             i < (SAND_SIM_WIDTH * SAND_SIM_HEIGHT);
             i = (uint16_t)(i + stride)) {
            if (!buffer_get_index(temp, i)) {
                continue;
            }

            uint8_t y = (uint8_t)(i / SAND_SIM_WIDTH);
            uint8_t x = (uint8_t)(i - (y * SAND_SIM_WIDTH));
            bool moved = false;
            int8_t nx = 0;
            int8_t ny = 0;

            for (uint8_t d = 0U; d < dir_count; d++) {
                nx = (int8_t)(x + dirs[d].x);
                ny = (int8_t)(y + dirs[d].y);

                if (!sim->bound_cb(nx, ny)) {
                    continue;
                }

                uint16_t next_index =
                    (uint16_t)((uint8_t)nx + ((uint8_t)ny * SAND_SIM_WIDTH));
                if (!SandSim_GetIndex(sim, next_index)) {
                    SandSim_SetIndex(sim, next_index, true);
                    SandSim_SetIndex(sim, i, false);
                    buffer_set_index(temp, i, false);
                    moved = true;
                    break;
                }
            }

            if (moved) {
                sim->set_cb(nx, ny, true);
            } else {
                sim->set_cb((int8_t)x, (int8_t)y, true);
            }
        }
    }
}

