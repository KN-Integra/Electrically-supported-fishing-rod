#include<zephyr/kernel.h>

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

        DESIRED_SPEED_TO_HIGH = 15,

        UNABLE_TO_SET_GPIO = 16
};


/// @brief Function initalising PWMs (drivers) and GPIOs.
/// @param speed_max_mrpm - max speed defined in mili RPM
/// @return error defined in error_codes
int init_pwm_motor_driver(uint32_t speed_max_mrpm);

/// @brief Set new desired speed
/// @param value - value in mili RPM
/// @return error defined in error_codes
int speed_set(uint32_t value);

/// @brief Get current actual speed (from encoders)  - TODO - implement with encoders
/// @param value - variable to save speed
/// @return error defined in error_codes
int speed_get(uint32_t* value);

/// @brief Update speed continuusly - TODO - implement with encoders
/// @return error defined in error_codes
int continuus_speed_update(void);

/// @brief Enter bootloader mode (in order to flash new software via nRF connect programmer) 
void enter_boot(void);

/// @brief Simple getter for max speed (set by init_pwm_motor_driver)
/// @return max speed in mili RPM
uint32_t get_current_max_speed(void);

// TODO - getter for is_initialised to optimise higher levels?
