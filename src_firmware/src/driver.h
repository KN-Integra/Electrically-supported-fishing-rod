/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2023 Maciej Baczmanski, Michal Kawiak, Jakub Mazur
 */

#include <zephyr/kernel.h>

// TODO - Add ERR before all error codes

/// @brief Error code definitions for motor driver
enum error_codes {
	SUCCESS = 0,
	NOT_INITIALISED = 1,

	PWM_DRV_NOT_READY = 2,
	GPIO_OUT_DIR_CNTRL_NOT_READY = 3,
	GPIO_IN_ENC_NOT_READY = 4,
	GPIO_OUT_BOOT_NOT_READY = 5,
	UNABLE_TO_SET_PWM_CHNL1 = 6,

	DESIRED_VALUE_TO_HIGH = 7,

	UNABLE_TO_SET_GPIO = 8,

	UNSUPPORTED_FUNCTION_IN_CURRENT_MODE = 9,
	VALUE_CONVERSION_ERROR = 10
};

/// @brief motor direction definitions
enum MotorDirection {
	FORWARD,
	BACKWARD
};

/// @brief Control Mode - whether excact speed or position is controlled
enum ControlModes {
	SPEED,
	POSITION
};

/// @brief struct representing current software version
struct DriverVersion {
	uint8_t major;
	uint8_t minor;
};

enum ChannelNumber {
	CH0,
	CH1
};

// TODO add preprocessor check for max supported channels (currently only CH0 and CH1)

enum PinNumber {
	P0,
	P1
};


/// @brief Function initalising PWMs (drivers) and GPIOs.
/// @param speed_max_mrpm - max speed defined in mili RPM
/// @return error defined in error_codes
int init_pwm_motor_driver(void);

/// @brief Set new desired (targeted) speed AND set the pwm
/// @param value - value in mili RPM
/// @return error defined in error_codes
int target_speed_set(uint32_t value, enum ChannelNumber chnl);

/// @brief Get current actual speed (from encoders)  - TODO - implement with encoders
/// @param value - variable to save speed
/// @return error defined in error_codes
int speed_get(enum ChannelNumber chnl, uint32_t *value);

/// @brief Turn motor on in proper direction, without modyfing target speed or set pwm output.
/// @param direction - FORWARD or BACKWARD
/// @return error defined in error_codes
int motor_on(enum MotorDirection direction, enum ChannelNumber chnl);

/// @brief Turn motor off, without modyfing pwm output
/// @return error defined in error_codes
int motor_off(enum ChannelNumber chnl);

/// @brief Simple getter for max speed (set by init_pwm_motor_driver)
/// @return max speed in mili RPM
uint32_t get_current_max_speed(void);

/// @brief Simple getter for current motor state (off or on)
/// @return true - motor is on; false - motor is off
bool get_motor_off_on(enum ChannelNumber chnl);

/// @brief Simple getter for firmware software version
/// @return DriverVersion struct of major and minor version
struct DriverVersion get_driver_version(void);

/// @brief Simple getter for target speed set with target_speed_set function
/// @return uint32_t representing target speed.
uint32_t speed_target_get(void);

/// @brief utility function for converting control mode as string to control mode as proper enum
/// @param str_control_mode control mode as string
/// @param ret_value output - control mode as control mode enum
/// @return error defined in error_codes
int get_control_mode_from_string(char *str_control_mode, enum ControlModes *ret_value);

/// @brief utility function for converting control mode from enum to string
/// @param control_mode control mode as enum
/// @param ret_value output - control mode as string
/// @return error defined in error_codes
int get_control_mode_as_string(enum ControlModes control_mode, char **ret_value);

int target_position_set(uint32_t new_target_position, enum ChannelNumber chnl);

int position_get(uint32_t *value, enum ChannelNumber chnl);

int mode_set(enum ControlModes new_mode);

int mode_get(enum ControlModes *value);

#if defined(CONFIG_BOARD_NRF52840DONGLE_NRF52840)
/// @brief Enter bootloader mode (in order to flash new software via nRF connect programmer)
void enter_boot(void);
#endif

uint64_t get_cycles_count_DEBUG(void);
uint64_t get_time_cycles_count_DEBUG(void);
int32_t get_ret_DEBUG(void);
uint32_t get_calc_speed_DEBUG(void);
uint32_t get_target_pos_DEBUG(void);
int32_t get_pos_d_DEBUG(void);
