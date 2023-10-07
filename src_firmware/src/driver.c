// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2023 Maciej Baczmanski, Michal Kawiak, Jakub Mazur

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/types.h>
#include <string.h>
#include "driver.h"
#include "button.h"


struct DriverVersion driver_ver = {
	.major = 1,
	.minor = 1,
};

/// SPEED control
static uint32_t target_speed_mrpm;// Target set by user
static uint32_t actual_mrpm; // actual speed calculated from encoder pins
static uint32_t max_mrpm;// max rpm set by user in init (read from documentation)
static uint32_t speed_control; // PID output -> pwm calculations input


/// ENCODER - variables used for actual speed calculation based on encoder pin and timer interrupts
static uint64_t count_cycles;
static uint64_t old_count_cycles;
static uint64_t count_timer;
struct k_timer my_timer;

/// BOOLS - drv init and on?
static bool drv_initialised; // was init command sent?
static bool is_motor_on; // was motor_on function called?

/// CONTROL MODE - whether speed or position is controlled
static enum ControlModes control_mode = SPEED;

/// POSITION control
static uint32_t current_position;
static uint32_t target_position;

static const uint32_t position_control_modifier = 10u; // accuracy of position control;
// if value is 10 position is controlled in deca-degrees; if 100 - centi-degrees etc.

static const uint32_t max_position = 360u * position_control_modifier;
static uint32_t encoder_cpr = 9600u; // TODO - add to Kconfig

/// PINS definitions
/// motor out
static const struct pwm_dt_spec pwm_motor_driver = PWM_DT_SPEC_GET(DT_ALIAS(pwm_drv_ch1));
static const struct gpio_dt_spec set_dir_p1 = GPIO_DT_SPEC_GET(DT_ALIAS(set_dir_p1_ch1), gpios);
static const struct gpio_dt_spec set_dir_p2 = GPIO_DT_SPEC_GET(DT_ALIAS(set_dir_p2_ch1), gpios);

/// enc in
static const struct gpio_dt_spec enc_p1_ch1 = GPIO_DT_SPEC_GET(DT_ALIAS(get_enc_p1_ch1), gpios);
static const struct gpio_dt_spec enc_p2_ch1 = GPIO_DT_SPEC_GET(DT_ALIAS(get_enc_p2_ch1), gpios);
static const struct gpio_dt_spec enc_ch1_pins[2] = {enc_p1_ch1, enc_p2_ch1};
static struct gpio_callback enc_ch1_cb[2];

#if defined(CONFIG_BOARD_NRF52840DONGLE_NRF52840)
static const struct gpio_dt_spec out_boot = GPIO_DT_SPEC_GET(DT_ALIAS(enter_boot_p), gpios);
#endif

static int32_t ret_debug = 100;// DEBUG ONLY

static int speed_pwm_set(uint32_t value)
{
	int ret;

	if (!drv_initialised) {
		return NOT_INITIALISED;
	}

	if (value > max_mrpm) {
		return DESIRED_VALUE_TO_HIGH;
	}

	if (target_speed_mrpm < max_mrpm/10) {
		value = 0;
		speed_control = 0;
	}

	uint64_t w_1 = pwm_motor_driver.period * (uint64_t)value;
	uint32_t w = value != 0 ? (uint32_t)(w_1/max_mrpm) : 0;

	ret = pwm_set_pulse_dt(&pwm_motor_driver, w);
	if (ret != 0) {
		return UNABLE_TO_SET_PWM_CHNL1;
	}

	return SUCCESS;
}

void update_speed_and_position_continuus(struct k_work *work)
{
	count_timer += 1; // debug only
	uint64_t diff = 0;

	// count encoder interrupts
	if (count_cycles > old_count_cycles) {
		diff = count_cycles - old_count_cycles;
	}
	old_count_cycles = count_cycles;

	// calculate actual position
	int32_t position_difference = (diff*max_position)/encoder_cpr;
	int32_t new_position = (int32_t)current_position + position_difference;

	if (new_position <= 0) {
		current_position = (uint32_t)(max_position + new_position);
	} else {
		current_position = ((uint32_t)new_position)%max_position;
	}

	// calculate actual speed
	actual_mrpm = RPM_TO_MRPM * MIN_TO_MS * diff /
		(CONFIG_ENC_STEPS_PER_ROTATION *
		 CONFIG_GEARSHIFT_RATIO *
		 CONFIG_ENC_TIMER_PERIOD_MS);
	int32_t speed_delta = target_speed_mrpm - actual_mrpm;

	// if motor is on, calculate control value
	if (get_motor_off_on()) {
		if (control_mode == SPEED) {
			int32_t speed_delta = target_speed_mrpm - actual_mrpm;

			int8_t Kp_numerator = 4;
			int8_t Kp_denominator = 10;

			int64_t temp_modifier_num = Kp_numerator*speed_delta;
			int32_t temp_modifier = (int32_t)(temp_modifier_num/Kp_denominator);

			// increase or decrese speed each iteration by Kp * speed_delta
			speed_control = (uint32_t)(speed_control + temp_modifier);

			ret_debug = speed_pwm_set(speed_control);
		} else if (control_mode == POSITION) {
			// TODO - it is temporary version of position control - will be improved!
			// TODO - control it in BOTH direction, currently it goes only forward
			int32_t position_delta = target_position - current_position;

			if (position_delta < 0) {
				position_delta = -position_delta;
			}
			if (position_delta > max_position/(36)) {
				//36 - control precision, TODO - possibly decrease the value?
				ret_debug = speed_pwm_set(max_mrpm/3);
				/*
				 * max_mrpm/3 is temporary,
				 * TODO - make this value dependent on position_delta,
				 * further the target, move faster
				 */
			} else {
				ret_debug = speed_pwm_set(0);
			}
		}
	}
}

K_WORK_DEFINE(speed_update_work, update_speed_and_position_continuus);

void my_timer_handler(struct k_timer *dummy)
{
	k_work_submit(&speed_update_work);
}

K_TIMER_DEFINE(my_timer, my_timer_handler, NULL);

void enc_ch1_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	count_cycles += 1;
}


int init_pwm_motor_driver(uint32_t speed_max_mrpm)
{
	max_mrpm = speed_max_mrpm;
	int ret;

	if (!device_is_ready(pwm_motor_driver.dev)) {
		return PWM_DRV_CHNL1_NOT_READY;
	}

	ret = pwm_set_pulse_dt(&pwm_motor_driver, 0);

	if (ret != 0) {
		return UNABLE_TO_SET_PWM_CHNL1;
	}


	if (!gpio_is_ready_dt(&set_dir_p1)) {
		return GPIO_OUT_DIR_CNTRL_1_CHNL1_NOT_READY;
	}

	ret = gpio_pin_configure_dt(&set_dir_p1, GPIO_OUTPUT_LOW);
	if (ret != 0) {
		return UNABLE_TO_SET_GPIO;
	}

	// TODO - move to function
	if (!gpio_is_ready_dt(&set_dir_p2)) {
		return GPIO_OUT_DIR_CNTRL_2_CHNL1_NOT_READY;
	}
	ret = gpio_pin_configure_dt(&set_dir_p2, GPIO_OUTPUT_LOW);


	if (ret != 0) {
		return UNABLE_TO_SET_GPIO;
	}

#if defined(CONFIG_BOARD_NRF52840DONGLE_NRF52840)
	if (!gpio_is_ready_dt(&out_boot)) {
		return GPIO_OUT_BOOT_NOT_READY;
	}
#endif

	for (int i = 0; i < 2; ++i) {
		ret = gpio_pin_configure_dt(&enc_ch1_pins[i], GPIO_INPUT);
		ret = gpio_pin_interrupt_configure_dt(&enc_ch1_pins[i], GPIO_INT_EDGE_BOTH);
		gpio_init_callback(&enc_ch1_cb[i], enc_ch1_callback, BIT(enc_ch1_pins[i].pin));
		gpio_add_callback(enc_ch1_pins[i].port, &enc_ch1_cb[i]);
		// TODO - ret error checking!!
	}

	k_timer_start(&my_timer, K_MSEC(CONFIG_ENC_TIMER_PERIOD_MS),
		      K_MSEC(CONFIG_ENC_TIMER_PERIOD_MS));

	off_on_button_init();

	drv_initialised = true;

	return SUCCESS;
}

int target_speed_set(uint32_t value)
{
	if (control_mode != SPEED) {
		return UNSUPPORTED_FUNCTION_IN_CURRENT_MODE;
	}
	target_speed_mrpm = value;
	count_cycles = 0;
	old_count_cycles = 0;
	return SUCCESS;
}

int speed_get(uint32_t *value)
{
	if (drv_initialised) {
		*value = actual_mrpm; // TODO - get speed from encoder
		return SUCCESS;
	}

	return NOT_INITIALISED;
}

int motor_on(enum MotorDirection direction)
{
	int ret;

	if (!drv_initialised) {
		return NOT_INITIALISED;
	}

	if (is_motor_on) {
		// Motor is already on, no need for starting it again
		return SUCCESS;
	}

	speed_control = 0;
	count_cycles = 0;
	old_count_cycles = 0;
	if (direction == FORWARD) {
		ret = gpio_pin_set_dt(&set_dir_p1, 1);
		if (ret != 0) {
			return UNABLE_TO_SET_GPIO;
		}
		ret = gpio_pin_set_dt(&set_dir_p2, 0);
		if (ret != 0) {
			return UNABLE_TO_SET_GPIO;
		}
	} else if (direction == BACKWARD) {
		ret = gpio_pin_set_dt(&set_dir_p1, 0);
		if (ret != 0) {
			return UNABLE_TO_SET_GPIO;
		}
		ret = gpio_pin_set_dt(&set_dir_p2, 1);
		if (ret != 0) {
			return UNABLE_TO_SET_GPIO;
		}
	}

	is_motor_on = true;
	return SUCCESS;
}

int motor_off(void)
{
	int ret;

	if (!drv_initialised) {
		return NOT_INITIALISED;
	}

	if (!is_motor_on) {
		// Motor is already off, no need for stopping it again
		return SUCCESS;
	}

	if (drv_initialised) {
		ret = gpio_pin_set_dt(&set_dir_p1, 0);

		if (ret != 0) {
			return UNABLE_TO_SET_GPIO;
		}
		ret = gpio_pin_set_dt(&set_dir_p2, 0);
		if (ret != 0) {
			return UNABLE_TO_SET_GPIO;
		}
		is_motor_on = false;
		return SUCCESS;
	}

	return NOT_INITIALISED;
}

uint32_t speed_target_get(void)
{
	return target_speed_mrpm;
}

#if defined(CONFIG_BOARD_NRF52840DONGLE_NRF52840)
void enter_boot(void)
{
	gpio_pin_configure_dt(&out_boot, GPIO_OUTPUT);
}
#endif

bool get_motor_off_on(void)
{
	return is_motor_on;
}

uint32_t get_current_max_speed(void)
{
	return max_mrpm;
}

int target_position_set(uint32_t new_target_position)
{
	if (!drv_initialised) {
		return NOT_INITIALISED;
	}

	if (control_mode != POSITION) {
		return UNSUPPORTED_FUNCTION_IN_CURRENT_MODE;
	}

	target_position = new_target_position;
	return SUCCESS;
}

int position_get(uint32_t *value)
{
	if (!drv_initialised) {
		return NOT_INITIALISED;
	}

	*value = current_position;
	return SUCCESS;
}

int mode_set(enum ControlModes new_mode)
{
	if (!drv_initialised) {
		return NOT_INITIALISED;
	}

	control_mode = new_mode;
	return SUCCESS;
}

int mode_get(enum ControlModes *value)
{
	if (!drv_initialised) {
		return NOT_INITIALISED;
	}

	*value = control_mode;
	return SUCCESS;
}

uint64_t get_cycles_count_DEBUG(void)
{
	return count_cycles;
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
	return speed_control;
}

struct DriverVersion get_driver_version(void)
{
	return driver_ver;
}

int get_control_mode_from_string(char *str_control_mode, enum ControlModes *ret_value)
{
	if (strcmp(str_control_mode, "speed") == 0) {
		*ret_value = SPEED;
		return SUCCESS;
	} else if (strcmp(str_control_mode, "position") == 0 ||
		  strcmp(str_control_mode, "pos") == 0) {

		*ret_value = POSITION;
		return SUCCESS;
	} else {
		return VALUE_CONVERSION_ERROR;
	}
}

int get_control_mode_as_string(enum ControlModes control_mode, char **ret_value)
{
	if (control_mode == SPEED) {
		*ret_value = "Speed";
		return SUCCESS;
	} else if (control_mode == POSITION) {
		*ret_value = "Position";
		return SUCCESS;
	} else {
		return VALUE_CONVERSION_ERROR;
	}
}
