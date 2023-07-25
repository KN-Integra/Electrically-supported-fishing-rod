#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>
#include "driver.h"







volatile static int32_t speed_mrpm = 0;
volatile static int32_t max_mrpm = 0;

volatile static int period = 0;

volatile static bool drv_initialised = false;

static const struct pwm_dt_spec pwm_motor_driver = PWM_DT_SPEC_GET(DT_ALIAS(pwm_drv));

static const struct gpio_dt_spec in1 = GPIO_DT_SPEC_GET(DT_ALIAS(in1), gpios);
static const struct gpio_dt_spec out_boot = GPIO_DT_SPEC_GET(DT_ALIAS(outboot), gpios);


int init_pwm_motor_driver(uint32_t speed_max_mrpm){
        period = pwm_motor_driver.period;
        max_mrpm = speed_max_mrpm;
        int ret;

        if (!device_is_ready(pwm_motor_driver.dev)) {
                return PWM_DRV_CHNL1_NOT_READY;
        }

        ret = pwm_set_pulse_dt(&pwm_motor_driver, period/2);

        if (0 != ret) {
                return UNABLE_TO_SET_PWM_CHNL1;
        }


        if (!gpio_is_ready_dt(&in1)) {
                return GPIO_OUT_DIR_CNTRL_1_CHNL1_NOT_READY;
	}

	ret = gpio_pin_configure_dt(&in1, GPIO_OUTPUT_ACTIVE);

        if (0 != ret) {
                return UNABLE_TO_SET_GPIO;
        }

        if (!gpio_is_ready_dt(&out_boot)) {
                return GPIO_OUT_BOOT_NOT_READY;
	}


        drv_initialised = true;

        return SUCCESS;
}

int speed_set(uint32_t value){
        int ret;
        speed_mrpm = value; // TODO implement encoder
        if(drv_initialised){

                if(value > max_mrpm){
                        return DESIRED_SPEED_TO_HIGH;
                }

                uint64_t w_1 = period * (uint64_t)value;
                uint32_t w = (uint32_t)(w_1/max_mrpm);

                ret = pwm_set_pulse_dt(&pwm_motor_driver, w);
                if(0 == ret){
                        return SUCCESS;
                }
                else{
                        return UNABLE_TO_SET_PWM_CHNL1;
                }
        }

        return NOT_INITIALISED;
}

int speed_get(uint32_t* value){
        if(drv_initialised){
                *value = speed_mrpm; // TODO - get speed from encoder
                return SUCCESS;
        }

        return NOT_INITIALISED;
}

int continuus_speed_update(void){
        if(drv_initialised){
                return SUCCESS;
        }

        return NOT_INITIALISED;
}


void enter_boot(void){
        gpio_pin_configure_dt(&out_boot, GPIO_OUTPUT);
}

uint32_t get_current_max_speed(void){
        return max_mrpm;
}