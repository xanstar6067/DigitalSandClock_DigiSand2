#ifndef INC_SAND_CLOCK_CONFIG_H_
#define INC_SAND_CLOCK_CONFIG_H_

#define SAND_CLOCK_PARTICLE_COUNT       55U
#define SAND_CLOCK_DEFAULT_SECONDS      60
#define SAND_CLOCK_DEFAULT_BRIGHTNESS   5
#define SAND_CLOCK_MAX_SECONDS          5999

#define SAND_CLOCK_TIME_POPUP_MS        3000U
#define SAND_CLOCK_BUTTON_DEBOUNCE_MS   25U
#define SAND_CLOCK_BUTTON_HOLD_MS       700U
#define SAND_CLOCK_BUTTON_STEP_MS       250U
#define SAND_CLOCK_SETTINGS_SAVE_DELAY_MS 2000U
#define SAND_CLOCK_MPU_POWERUP_DELAY_MS  2000U

/*
 * STM32G031F6P6 has 32 KB flash arranged as 16 pages of 2 KB. The last page
 * (page 15, 0x08007800..0x08007FFF) is reserved for settings storage. Keep
 * the linker FLASH length in STM32G031F6PX_FLASH.ld limited to 30 KB.
 */
#define SAND_CLOCK_SETTINGS_FLASH_ADDRESS 0x08007800UL
#define SAND_CLOCK_SETTINGS_FLASH_PAGE    15U

#define SAND_CLOCK_LED_ACTIVE_LOW       1

/*
 * The WeAct board KEY connects the shared PA14/PA15 package pad to 3.3 V.
 * Its external 10 kOhm resistor pulls the line down when the key is released.
 */
#define SAND_CLOCK_UKEY_ACTIVE_LOW      0

/*
 * MPU axis mapping. These values reproduce the original assembly:
 *   screen X <- raw Y
 *   screen Y <- raw Z
 *   depth    <- raw X
 * Flip signs or axes here if the sensor is mounted differently.
 */
#define SAND_CLOCK_IMU_X_AXIS           1U
#define SAND_CLOCK_IMU_X_SIGN           (1)
#define SAND_CLOCK_IMU_Y_AXIS           2U
#define SAND_CLOCK_IMU_Y_SIGN           (1)
#define SAND_CLOCK_IMU_Z_AXIS           0U
#define SAND_CLOCK_IMU_Z_SIGN           (1)

/*
 * Display orientation helpers. Keep them at 0 first; they are intentionally
 * simple switches for the common cases after the hardware is alive.
 */
#define SAND_CLOCK_DISPLAY_MIRROR_X     0
#define SAND_CLOCK_DISPLAY_MIRROR_Y     0

/*
 * The time popup is viewed in the opposite orientation to the internal
 * MAX7219 coordinate system. Rotating the complete 16x8 popup also swaps the
 * two physical modules: minutes are shown above seconds.
 */
#define SAND_CLOCK_TIME_DISPLAY_ROTATE_180 1

#endif /* INC_SAND_CLOCK_CONFIG_H_ */
