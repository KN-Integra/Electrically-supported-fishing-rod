// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2023 Maciej Baczmanski, Michal Kawiak, Jakub Mazur

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/types.h>
#include <string.h>
#include "return_codes.h"
#include "driver.h"

static const struct DriverVersion driver_ver = {
	.major = 2,
	.minor = 0,
};

// TODO - correct all of the CH0 to actual channel!!!!!!!!

/// temporary debug only variables: - To be deleted after developemnt is finished!
static uint64_t count_timer;
static int32_t ret_debug = 100;// DEBUG ONLY

#pragma region InternalFunctions// declarations of internal functions
static void enc_callback_wrapper(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
static int speed_pwm_set(uint32_t value);
static void update_speed_and_position_continuous(struct k_work *work);
static void continuous_calculation_timer_handler(struct k_timer *dummy);
static void enc_callback(enum ChannelNumber chnl, enum PinNumber pin);
static int set_direction_raw(enum MotorDirection direction, enum ChannelNumber chnl);
#pragma endregion InternalFunctions

/// Unit conversion defines
#define MIN_TO_MS 60000
#define RPM_TO_MRPM 1000
// TODO - add define for pins per channel

/// CONTROL MODE - whether speed or position is controlled
static enum ControlModes control_mode = SPEED;
/// encoder timer - timer is common for both channels
struct k_timer continuous_calculation_timer;
/// driver initialized (Was init function called)?
static bool drv_initialised;

// Struct representing singular channel, including channel - dependent variables
struct DriverChannel {
	/// PINS definitions
	/// motor out
	const struct pwm_dt_spec pwm_motor_driver;
	const struct gpio_dt_spec set_dir_pins[2];
	/// enc in
	const struct gpio_dt_spec enc_pins[2];
	struct gpio_callback enc_cb;

	/// ENCODER - variables used for actual speed calculation based on encoder pin and
	// timer interrupts
	uint64_t count_cycles;
	uint64_t old_count_cycles;

	/// SPEED control
	uint32_t target_speed_mrpm;// Target set by user
	uint32_t actual_mrpm; // actual speed calculated from encoder pins
	uint32_t speed_control; // PID output -> pwm calculations input

	/// POSITION control
	uint32_t curr_pos;
	uint32_t target_position;

	int32_t position_delta;//TEMP

	const uint32_t max_pos;

	bool prev_val_enc[2];
	enum MotorDirection actual_dir;

	/// BOOLS - is motor on?
	bool is_motor_on; // was motor_on function called?
};
static struct DriverChannel drv_chnls[CONFIG_SUPPORTED_CHANNEL_NUMBER] = {
	{
		.max_pos = 360u * CONFIG_POSITION_CONTROL_MODIFIER,
		.pwm_motor_driver = PWM_DT_SPEC_GET(DT_ALIAS(pwm_drv_ch1)),
		.set_dir_pins[P0] = GPIO_DT_SPEC_GET(DT_ALIAS(set_dir_p1_ch1), gpios),
		.set_dir_pins[P1] = GPIO_DT_SPEC_GET(DT_ALIAS(set_dir_p2_ch1), gpios),
		.enc_pins[P0] = GPIO_DT_SPEC_GET(DT_ALIAS(get_enc_p1_ch1), gpios),
		.enc_pins[P1] = GPIO_DT_SPEC_GET(DT_ALIAS(get_enc_p2_ch1), gpios),
		.actual_dir = STOPPED
	}
};

#if defined(CONFIG_BOARD_NRF52840DONGLE_NRF52840) // BOOT functionality
static const struct gpio_dt_spec out_boot = GPIO_DT_SPEC_GET(DT_ALIAS(enter_boot_p), gpios);
void enter_boot(void)
{
	gpio_pin_configure_dt(&out_boot, GPIO_OUTPUT);
}
#endif

// function implementations:
#pragma region TimerWorkCallback // timer interrupt internal functions
static void update_speed_and_position_continuous(struct k_work *work)
{
	for (enum ChannelNumber chnl = 0; chnl < CONFIG_SUPPORTED_CHANNEL_NUMBER; ++chnl) {
		count_timer += 1; // debug only

		// TODO - move declarations to global scope for efficiency (or make them static?)

		uint64_t diff = 0;

		// count encoder interrupts between timer interrupts:
		if (drv_chnls[chnl].count_cycles > drv_chnls[chnl].old_count_cycles) {
			diff = drv_chnls[chnl].count_cycles - drv_chnls[chnl].old_count_cycles;
		}
		drv_chnls[chnl].old_count_cycles = drv_chnls[chnl].count_cycles;

		// calculate actual position
		int32_t pos_diff = (diff*drv_chnls[chnl].max_pos) /
				   (CONFIG_ENC_STEPS_PER_ROTATION * CONFIG_GEARSHIFT_RATIO);
		int32_t new_pos = (int32_t)drv_chnls[CH0].curr_pos + pos_diff;

		if (new_pos <= 0) {
			drv_chnls[chnl].curr_pos = (uint32_t)(drv_chnls[chnl].max_pos + new_pos);
		} else {
			drv_chnls[chnl].curr_pos = ((uint32_t)new_pos)%(drv_chnls[chnl].max_pos);
		}

		// calculate actual speed
		drv_chnls[chnl].actual_mrpm = RPM_TO_MRPM * MIN_TO_MS * diff /
			(CONFIG_ENC_STEPS_PER_ROTATION *
			CONFIG_GEARSHIFT_RATIO *
			CONFIG_ENC_TIMER_PERIOD_MS);

		// if motor is on, calculate control value
		if (get_motor_off_on(chnl)) {
			int ret;

			if (control_mode == SPEED) {
				int32_t speed_delta = drv_chnls[chnl].target_speed_mrpm -
						      drv_chnls[chnl].actual_mrpm;

				int64_t temp_modifier_num = CONFIG_KP_NUMERATOR_FOR_SPEED *
							    speed_delta;
				int32_t temp_modifier = (int32_t)(temp_modifier_num /
								  CONFIG_KP_DENOMINATOR_FOR_SPEED);

				// increase or decrese speed each iteration by Kp * speed_delta
				drv_chnls[chnl].speed_control = (uint32_t)
								(drv_chnls[chnl].speed_control +
								 temp_modifier);

				// Cap control at max rpm speed,
				// to avoid cumulation of too high speeds.
				if (drv_chnls[chnl].speed_control > CONFIG_SPEED_MAX_MRPM) {
					drv_chnls[chnl].speed_control = CONFIG_SPEED_MAX_MRPM;
				}

				ret = speed_pwm_set(drv_chnls[chnl].speed_control);
			} else if (control_mode == POSITION) {
				// TODO - it is temporary version of position control -
				// - will be improved!
				// TODO - control it in BOTH direction, currently
				// it goes only forward
				drv_chnls[chnl].position_delta = drv_chnls[chnl].target_position -
								 drv_chnls[chnl].curr_pos;

				if (drv_chnls[chnl].position_delta < 0) {
					drv_chnls[chnl].position_delta =
					-drv_chnls[chnl].position_delta;
				}

				if (drv_chnls[chnl].position_delta >
						drv_chnls[CH0].max_pos/(36)) {
					//36 - control precision, TODO -
					// possibly decrease the value?
					drv_chnls[chnl].target_speed_mrpm = CONFIG_SPEED_MAX_MRPM/3;
					/*
					 * CONFIG_SPEED_MAX_MRPM/3 is temporary,
					 * TODO - make this value dependent on position_delta,
					 * further the target, move faster
					 */
				} else {
					drv_chnls[chnl].target_speed_mrpm = 0;
				}
				ret_debug = speed_pwm_set(drv_chnls[chnl].target_speed_mrpm);
			}
		}
	}
}
K_WORK_DEFINE(speed_and_position_update_work, update_speed_and_position_continuous);
static void continuous_calculation_timer_handler(struct k_timer *dummy)
{
	k_work_submit(&speed_and_position_update_work);
}
K_TIMER_DEFINE(continuous_calculation_timer, continuous_calculation_timer_handler, NULL);
#pragma endregion TimerWorkCallback
// encoder functions
static void enc_callback(enum ChannelNumber chnl, enum PinNumber pin)
{
	// TODO - maybe split this callback to two callbacks, for P1 and P0:
	// what if motor is stuck and hitting one enc. from two directions?
	// How will speed calculation behave if motor will keep changing position?
	// (ex. when setting position?)
	drv_chnls[chnl].count_cycles += 1;

	if (drv_chnls[chnl].actual_mrpm == 0) {
		drv_chnls[chnl].actual_dir = STOPPED;
		return;
	}

	if (pin == P0) {
		if (drv_chnls[chnl].prev_val_enc[P0] == drv_chnls[chnl].prev_val_enc[P1]) {
			drv_chnls[chnl].actual_dir = BACKWARD;
		} else {
			drv_chnls[chnl].actual_dir = FORWARD;
		}
	}
}
static void enc_callback_wrapper(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	for (int channel = 0; channel < CONFIG_SUPPORTED_CHANNEL_NUMBER; ++channel) {
		for (int pin = 0; pin < 2; ++pin) {
			if (pins & BIT(drv_chnls[channel].enc_pins[pin].pin)) {
				drv_chnls[channel].prev_val_enc[pin] =
					gpio_pin_get(dev, drv_chnls[channel].enc_pins[pin].pin);
				enc_callback(channel, pin);
			}
		}
	}
}

// init motor
void init_pwm_motor_driver(void)
{

	for (unsigned int channel = 0; channel < CONFIG_SUPPORTED_CHANNEL_NUMBER; channel++) {

		__ASSERT(device_is_ready(drv_chnls[channel].pwm_motor_driver.dev),
			 "PWM Driver not ready!");

		__ASSERT_EVAL((void) pwm_set_pulse_dt(&(drv_chnls[channel].pwm_motor_driver), 0),
			int r = pwm_set_pulse_dt(&(drv_chnls[channel].pwm_motor_driver), 0),
			!r, "Unable to set PWM!");

		for (uint8_t pin = 0; pin < 2; ++pin) {
			__ASSERT(gpio_is_ready_dt(&(drv_chnls[channel].set_dir_pins[pin])),
				 "Direction Pin %d on channel %d not ready!",
				 pin, channel);

			__ASSERT_EVAL((void) gpio_pin_configure_dt(
							&(drv_chnls[channel].set_dir_pins[pin]),
							GPIO_OUTPUT_LOW),
				int r = gpio_pin_configure_dt(
							&(drv_chnls[channel].set_dir_pins[pin]),
							GPIO_OUTPUT_LOW),
				!r, "Unable to configure direction Pin %d on channel %d!",
				pin, channel);
		}

#if defined(CONFIG_BOARD_NRF52840DONGLE_NRF52840)
		__ASSERT(gpio_is_ready_dt(&out_boot),
			 "GPIO Boot pin not ready!");
#endif

		for (int pin = 0; pin < 2; ++pin) {
			__ASSERT(gpio_is_ready_dt(&(drv_chnls[channel].enc_pins[pin])),
				 "Encoder Pin %d on channel %d not ready!",
				 pin, channel);

			__ASSERT_EVAL((void) gpio_pin_configure_dt(
							&(drv_chnls[channel].enc_pins[pin]),
							GPIO_INPUT),
				int r = gpio_pin_configure_dt(
							&(drv_chnls[channel].enc_pins[pin]),
							GPIO_INPUT),
				!r, "Unable to configure Encoder pin %d on channel %d!",
				pin, channel);

			__ASSERT_EVAL((void) gpio_pin_interrupt_configure_dt(
							&(drv_chnls[channel].enc_pins[pin]),
							GPIO_INT_EDGE_BOTH),
				int r = gpio_pin_interrupt_configure_dt(
							&(drv_chnls[channel].enc_pins[pin]),
							GPIO_INT_EDGE_BOTH),
				!r, "Unable to configure Encoder pin %d on channel %d!",
				pin, channel);

			drv_chnls[channel].prev_val_enc[pin] =
				gpio_pin_get(drv_chnls[channel].enc_pins[pin].port,
							 drv_chnls[channel].enc_pins[pin].pin);
		}

		gpio_init_callback(&(drv_chnls[channel].enc_cb),
					enc_callback_wrapper,
					BIT(drv_chnls[channel].enc_pins[P0].pin) |
					BIT(drv_chnls[channel].enc_pins[P1].pin));
		gpio_add_callback(drv_chnls[channel].enc_pins[P0].port,
					&(drv_chnls[channel].enc_cb));
	}

	k_timer_start(&continuous_calculation_timer, K_MSEC(CONFIG_ENC_TIMER_PERIOD_MS),
		      K_MSEC(CONFIG_ENC_TIMER_PERIOD_MS));

	drv_initialised = true;
}

return_codes_t motor_on(enum MotorDirection direction, enum ChannelNumber chnl)
{
	int ret;

	if (!drv_initialised) {
		return ERR_NOT_INITIALISED;
	}

	// TODO - change direction during spinning

	if (drv_chnls[chnl].is_motor_on) {
		// Motor is already on, no need for starting it again
		return SUCCESS;
	}

	drv_chnls[chnl].speed_control = 0;
	drv_chnls[chnl].count_cycles = 0;
	drv_chnls[chnl].old_count_cycles = 0;
	
	ret = set_direction_raw(direction, chnl);
	if(ret != SUCCESS) {
		return ret;
	}

	drv_chnls[chnl].is_motor_on = true;
	return SUCCESS;
}
return_codes_t motor_off(enum ChannelNumber chnl)
{
	int ret;

	if (!drv_initialised) {
		return ERR_NOT_INITIALISED;
	}

	if (!drv_chnls[chnl].is_motor_on) {
		// Motor is already off, no need for stopping it again
		return SUCCESS;
	}

	if (drv_initialised) {
		ret = gpio_pin_set_dt(&(drv_chnls[chnl].set_dir_pins[P0]), 0);

		if (ret != 0) {
			return ERR_UNABLE_TO_SET_GPIO;
		}
		ret = gpio_pin_set_dt(&(drv_chnls[chnl].set_dir_pins[P1]), 0);
		if (ret != 0) {
			return ERR_UNABLE_TO_SET_GPIO;
		}
		drv_chnls[chnl].is_motor_on = false;
		return SUCCESS;
	}

	return ERR_NOT_INITIALISED;
}
bool get_motor_off_on(enum ChannelNumber chnl)
{
	return drv_chnls[chnl].is_motor_on;
}

static int set_direction_raw(enum MotorDirection direction, enum ChannelNumber chnl){
	int ret;
	// TODO, combine if and else in some smart way, as they are very similar
	if (direction == FORWARD) {
		ret = gpio_pin_set_dt(&(drv_chnls[chnl].set_dir_pins[P0]), 1);
		if (ret != 0) {
			return ERR_UNABLE_TO_SET_GPIO;
		}
		ret = gpio_pin_set_dt(&(drv_chnls[chnl].set_dir_pins[P1]), 0);
		if (ret != 0) {
			return ERR_UNABLE_TO_SET_GPIO;
		}
	} else if (direction == BACKWARD) {
		ret = gpio_pin_set_dt(&(drv_chnls[chnl].set_dir_pins[P0]), 0);
		if (ret != 0) {
			return ERR_UNABLE_TO_SET_GPIO;
		}
		ret = gpio_pin_set_dt(&(drv_chnls[chnl].set_dir_pins[P1]), 1);
		if (ret != 0) {
			return ERR_UNABLE_TO_SET_GPIO;
		}
	}
	return SUCCESS;
}

int get_motor_actual_direction(enum ChannelNumber chnl, enum MotorDirection *out_dir) {
	if (!drv_initialised) {
		return ERR_NOT_INITIALISED;
	}

	*out_dir = drv_chnls[chnl].actual_dir;

	return SUCCESS;
}

static int speed_pwm_set(uint32_t value)
{
	int ret;

	if (!drv_initialised) {
		return ERR_NOT_INITIALISED;
	}

	if (value > CONFIG_SPEED_MAX_MRPM) {
		return ERR_DESIRED_VALUE_TO_HIGH;
	}

	if (drv_chnls[CH0].target_speed_mrpm < CONFIG_SPEED_MAX_MRPM / 10) {
		value = 0;
		drv_chnls[CH0].speed_control = 0;
	}

	uint64_t w_1 = drv_chnls[CH0].pwm_motor_driver.period * (uint64_t)value;
	uint32_t w = value != 0 ? (uint32_t)(w_1 / CONFIG_SPEED_MAX_MRPM) : 0;

	ret = pwm_set_pulse_dt(&(drv_chnls[CH0].pwm_motor_driver), w);
	if (ret != 0) {
		return ERR_UNABLE_TO_SET_PWM;
	}

	return SUCCESS;
}
return_codes_t target_speed_set(uint32_t value, enum ChannelNumber chnl)
{
	if (control_mode != SPEED) {
		return ERR_UNSUPPORTED_FUNCTION_IN_CURRENT_MODE;
	}
	drv_chnls[chnl].target_speed_mrpm = value;
	drv_chnls[chnl].count_cycles = 0;
	drv_chnls[chnl].old_count_cycles = 0;
	return SUCCESS;
}
return_codes_t speed_get(enum ChannelNumber chnl, uint32_t *value)
{
	if (drv_initialised) {
		*value = drv_chnls[chnl].actual_mrpm; // TODO - get speed from encoder
		return SUCCESS;
	}

	return ERR_NOT_INITIALISED;
}
uint32_t speed_target_get(void)
{
	return drv_chnls[CH0].target_speed_mrpm;
}
uint32_t get_current_max_speed(void)
{
	return CONFIG_SPEED_MAX_MRPM;
}

return_codes_t target_position_set(uint32_t new_target_position, enum ChannelNumber chnl)
{
	if (!drv_initialised) {
		return ERR_NOT_INITIALISED;
	}

	if (control_mode != POSITION) {
		return ERR_UNSUPPORTED_FUNCTION_IN_CURRENT_MODE;
	}

	drv_chnls[chnl].target_position = new_target_position;
	return SUCCESS;
}
return_codes_t position_get(uint32_t *value, enum ChannelNumber chnl)
{
	if (!drv_initialised) {
		return ERR_NOT_INITIALISED;
	}

	*value = drv_chnls[chnl].curr_pos;
	return SUCCESS;
}

return_codes_t mode_set(enum ControlModes new_mode)
{
	if (!drv_initialised) {
		return ERR_NOT_INITIALISED;
	}

	control_mode = new_mode;
	return SUCCESS;
}
return_codes_t mode_get(enum ControlModes *value)
{
	if (!drv_initialised) {
		return ERR_NOT_INITIALISED;
	}

	*value = control_mode;
	return SUCCESS;
}

// TODO - remove, use shell arguments instead
return_codes_t get_control_mode_from_string(char *str_control_mode, enum ControlModes *ret_value)
{
	if (strcmp(str_control_mode, "speed") == 0) {
		*ret_value = SPEED;
		return SUCCESS;
	} else if (strcmp(str_control_mode, "position") == 0 ||
		  strcmp(str_control_mode, "pos") == 0) {

		*ret_value = POSITION;
		return SUCCESS;
	} else {
		return ERR_VALUE_CONVERSION_ERROR;
	}
}

// TODO - remove, use shell arguments instead
return_codes_t get_control_mode_as_string(enum ControlModes control_mode, char **ret_value)
{
	if (control_mode == SPEED) {
		*ret_value = "Speed";
		return SUCCESS;
	} else if (control_mode == POSITION) {
		*ret_value = "Position";
		return SUCCESS;
	} else {
		return ERR_VALUE_CONVERSION_ERROR;
	}
}

#pragma region DebugFunctions
uint64_t get_cycles_count_DEBUG(void)
{
	return drv_chnls[CH0].count_cycles;
}

uint64_t get_time_cycles_count_DEBUG(void)
{
	return count_timer;
}

int32_t get_ret_DEBUG(void)
{
	return ret_debug;
}

uint32_t get_calc_speed_DEBUG(void)
{
	return drv_chnls[CH0].speed_control;
}

uint32_t get_target_pos_DEBUG(void)
{
	return drv_chnls[CH0].target_position;
}

int32_t get_pos_d_DEBUG(void)
{
	return drv_chnls[CH0].position_delta;
}
#pragma endregion DebugFunctions

struct DriverVersion get_driver_version(void)
{
	return driver_ver;
}
