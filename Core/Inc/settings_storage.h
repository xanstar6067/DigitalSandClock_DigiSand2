#ifndef INC_SETTINGS_STORAGE_H_
#define INC_SETTINGS_STORAGE_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int16_t seconds;
    uint8_t brightness;
} SettingsStorageData;

bool SettingsStorage_Load(SettingsStorageData *settings);
bool SettingsStorage_Save(const SettingsStorageData *settings);

#endif /* INC_SETTINGS_STORAGE_H_ */

