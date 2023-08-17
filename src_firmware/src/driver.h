#include<zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/// @brief Error code definitions for motor driver
enum error_codes{
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

        DESIRED_VALUE_TO_HIGH = 15,

        UNABLE_TO_SET_GPIO = 16,

        WRONG_MODE = 17
};


typedef enum ControlModes{
        SPEED = 0,
        POSITION = 1
} ControlModes;



/// @brief Function initalising PWMs (drivers) and GPIOs.
/// @param speed_max_mrpm - max speed defined in mili RPM
/// @param default_control_mode - control mode (pass SPEED for default behaviour)
/// @param start_position - position at which motor starts (pass 0 for default behaviour; used only in position control)
/// @return error defined in error_codes
int init_pwm_motor_driver(uint32_t speed_max_mrpm, ControlModes default_control_mode, uint32_t start_position);

/// @brief Set new desired (targeted) speed AND set the pwm
/// @param value - value in mili RPM
/// @return error defined in error_codes
int target_speed_set(uint32_t value);

/// @brief Get current actual speed (from encoders)  - TODO - implement with encoders
/// @param value - variable to save speed
/// @return error defined in error_codes
int speed_get(uint32_t* value);


int target_position_set(uint32_t new_target_position);

int position_get(uint32_t* value);

int mode_set(ControlModes new_mode);

int mode_get(ControlModes* value);


/// @brief Enter bootloader mode (in order to flash new software via nRF connect programmer) 
void enter_boot(void);

/// @brief Simple getter for max speed (set by init_pwm_motor_driver)
/// @return max speed in mili RPM
uint32_t get_current_max_speed(void);

uint64_t get_cycles_count_DEBUG(void);
uint64_t get_time_cycles_count_DEBUG(void);
int32_t get_ret_DEBUG(void);

char* get_driver_version(void);

int get_control_mode_from_string(char* str_control_mode, ControlModes* ret_value);
