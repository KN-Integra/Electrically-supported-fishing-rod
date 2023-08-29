/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2023 Maciej Baczmanski, Michal Kawiak, Jakub Mazur
 */

#include <zephyr/kernel.h>

#define MIN_TO_MS 60000
#define RPM_TO_MRPM 1000

/// @brief Error code definitions for motor driver
enum error_codes {
	SUCCESS = 0,
	NOT_INITIALISED = 1,

	PWM_DRV_CHNL1_NOT_READY = 2,
	GPIO_OUT_DIR_CNTRL_1_CHNL1_NOT_READY = 3,
	GPIO_OUT_DIR_CNTRL_2_CHNL1_NOT_READY = 4,
	GPIO_IN_ENC_1_CHNL1_NOT_READY = 5,
	GPIO_IN_ENC_2_CHNL1_NOT_READY = 6,

	PWM_DRV_CHNL2_NOT_READY = 7,
	GPIO_OUT_DIR_CNTRL_1_CHNL2_NOT_READY = 8,
	GPIO_OUT_DIR_CNTRL_2_CHNL2_NOT_READY = 9,
	GPIO_IN_ENC_1_CHNL2_NOT_READY = 10,
	GPIO_IN_ENC_2_CHNL2_NOT_READY = 11,

	GPIO_OUT_BOOT_NOT_READY = 12,

	UNABLE_TO_SET_PWM_CHNL1 = 13,
	UNABLE_TO_SET_PWM_CHNL2 = 14,

	DESIRED_SPEED_TO_HIGH = 15,

	UNABLE_TO_SET_GPIO = 16
};

/// @brief motor direction definitions
enum MotorDirection {
	FORWARD,
	BACKWARD
};

/// @brief struct representing current software version
struct DriverVersion {
	uint8_t major;
	uint8_t minor;
};

/// @brief Function initalising PWMs (drivers) and GPIOs.
/// @param speed_max_mrpm - max speed defined in mili RPM
/// @return error defined in error_codes
int init_pwm_motor_driver(uint32_t speed_max_mrpm);

/// @brief Set new desired (targeted) speed AND set the pwm
/// @param value - value in mili RPM
/// @return error defined in error_codes
int target_speed_set(uint32_t value);

/// @brief Get current actual speed (from encoders)  - TODO - implement with encoders
/// @param value - variable to save speed
/// @return error defined in error_codes
int speed_get(uint32_t *value);

/// @brief Turn motor on in proper direction, without modyfing target speed or set pwm output.
/// @param direction - FORWARD or BACKWARD
/// @return error defined in error_codes
int motor_on(enum MotorDirection direction);

/// @brief Turn motor off, without modyfing pwm output
/// @return error defined in error_codes
int motor_off(void);

/// @brief Simple getter for max speed (set by init_pwm_motor_driver)
/// @return max speed in mili RPM
uint32_t get_current_max_speed(void);

/// @brief Simple getter for current motor state (off or on)
/// @return true - motor is on; false - motor is off
bool get_motor_off_on(void);

/// @brief Simple getter for firmware software version
/// @return DriverVersion struct of major and minor version
struct DriverVersion get_driver_version(void);

/// @brief Simple getter for target speed set with target_speed_set function
/// @return uint32_t representing target speed.
uint32_t speed_target_get(void);

#if defined(CONFIG_BOARD_NRF52840DONGLE_NRF52840)
/// @brief Enter bootloader mode (in order to flash new software via nRF connect programmer)
void enter_boot(void);
#endif

uint64_t get_cycles_count_DEBUG(void);
uint64_t get_time_cycles_count_DEBUG(void);
int32_t get_ret_DEBUG(void);
uint32_t get_calc_speed_DEBUG(void);
