#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/types.h>
#include "driver.h"
#include "button.h"

//static char* driver_ver = "0.1";

DriverVersion driver_ver = {
        .major = 0,
        .minor = 1,
};

volatile static uint32_t target_speed_mrpm = 0;// Target set by user
volatile static uint32_t actual_mrpm = 0; // actual speed calculated from encoder pins

volatile static uint32_t max_mrpm = 0;// max rpm set by user in init (read from documentation)

static uint32_t speed_control = 0; // temporary speed used for PID calculations


/// variables used for actual speed calculation based on encoder pin and timer interrupts
static uint64_t count_cycles = 0;
static uint64_t old_count_cycles = 0;
static uint64_t count_timer = 0;
struct k_timer my_timer;
///

volatile static bool drv_initialised = false; // was init command sent?

volatile static bool is_motor_on = false; // was motor_on function called?


static const struct pwm_dt_spec pwm_motor_driver = PWM_DT_SPEC_GET(DT_ALIAS(pwm_drv_ch1));

static const struct gpio_dt_spec set_dir_p1 = GPIO_DT_SPEC_GET(DT_ALIAS(set_dir_p1_ch1), gpios);
static const struct gpio_dt_spec set_dir_p2 = GPIO_DT_SPEC_GET(DT_ALIAS(set_dir_p2_ch1), gpios);

#if defined(CONFIG_BOARD_NRF52840DONGLE_NRF52840)
static const struct gpio_dt_spec out_boot = GPIO_DT_SPEC_GET(DT_ALIAS(enter_boot_p), gpios);
#endif

static const struct gpio_dt_spec enc_p1_ch1 = GPIO_DT_SPEC_GET(DT_ALIAS(get_enc_p1_ch1), gpios);
static const struct gpio_dt_spec enc_p2_ch1 = GPIO_DT_SPEC_GET(DT_ALIAS(get_enc_p2_ch1), gpios);

static const struct gpio_dt_spec enc_ch1_pins[2] = {enc_p1_ch1, enc_p2_ch1};

static struct gpio_callback enc_ch1_cb[2];


static int32_t ret_debug = 100;// DEBUG ONLY


void update_speed_continuus(struct k_work *work)
{
        count_timer+=1;
        uint64_t diff = 0;
        if(count_cycles > old_count_cycles){
                diff = count_cycles - old_count_cycles;
        }
        old_count_cycles = count_cycles;

        actual_mrpm = (diff*25)/4; //(diff*60)/(64*150) * 1000;
        int32_t speed_delta = target_speed_mrpm - actual_mrpm;

        if(get_motor_off_on()){
                int8_t Kp_numerator = 4;
                int8_t Kp_denominator = 10;

                int64_t temp_modifier_num = Kp_numerator*speed_delta;
                int32_t temp_modifier = (int32_t)(temp_modifier_num/Kp_denominator);

                speed_control = (uint32_t)(speed_control + temp_modifier); // increase or decrese speed each iteration by Kp * speed_delta

                ret_debug = speed_pwm_set(speed_control);
        }
}

K_WORK_DEFINE(speed_update_work, update_speed_continuus);

void my_timer_handler(struct k_timer *dummy){
    k_work_submit(&speed_update_work);
}

K_TIMER_DEFINE(my_timer, my_timer_handler, NULL);

void enc_ch1_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins){
        count_cycles +=1;
}


int init_pwm_motor_driver(uint32_t speed_max_mrpm){
        max_mrpm = speed_max_mrpm;
        int ret;

        if (!device_is_ready(pwm_motor_driver.dev)) {
                return PWM_DRV_CHNL1_NOT_READY;
        }

        ret = pwm_set_pulse_dt(&pwm_motor_driver, 0);

        if (0 != ret) {
                return UNABLE_TO_SET_PWM_CHNL1;
        }


        if (!gpio_is_ready_dt(&set_dir_p1)) {
                return GPIO_OUT_DIR_CNTRL_1_CHNL1_NOT_READY;
	}

        ret = gpio_pin_configure_dt(&set_dir_p1, GPIO_OUTPUT_LOW);
        if (0 != ret) {
                return UNABLE_TO_SET_GPIO;
        }

        // TODO - move to function
        if (!gpio_is_ready_dt(&set_dir_p2)) {
                return GPIO_OUT_DIR_CNTRL_2_CHNL1_NOT_READY;
	}
        ret = gpio_pin_configure_dt(&set_dir_p2, GPIO_OUTPUT_LOW);


        if (0 != ret) {
                return UNABLE_TO_SET_GPIO;
        }

#if defined(CONFIG_BOARD_NRF52840DONGLE_NRF52840)
        if (!gpio_is_ready_dt(&out_boot)) {
                return GPIO_OUT_BOOT_NOT_READY;
	}
#endif

        for(int i = 0; i<2; ++i){
                ret = gpio_pin_configure_dt(&enc_ch1_pins[i], GPIO_INPUT);
                ret = gpio_pin_interrupt_configure_dt(&enc_ch1_pins[i], GPIO_INT_EDGE_BOTH);
                gpio_init_callback(&enc_ch1_cb[i], enc_ch1_callback, BIT(enc_ch1_pins[i].pin));
                gpio_add_callback(enc_ch1_pins[i].port, &enc_ch1_cb[i]);
                // TODO - ret error checking!!
        }

        k_timer_start(&my_timer, K_SECONDS(1), K_SECONDS(1));

        off_on_button_init();

        drv_initialised = true;

        return SUCCESS;
}

int target_speed_set(uint32_t value){
        target_speed_mrpm = value;
        count_cycles = 0;
        old_count_cycles = 0;
        return SUCCESS;
}

int speed_pwm_set(uint32_t value){
        int ret;
        if(!drv_initialised){
                return NOT_INITIALISED;
        }

                if(value > max_mrpm){
                        return DESIRED_SPEED_TO_HIGH;
                }

        if(target_speed_mrpm < max_mrpm/10){
                value = 0;
                speed_control = 0;
        }

        uint64_t w_1 = pwm_motor_driver.period * (uint64_t)value;
        uint32_t w = value!=0 ? (uint32_t)(w_1/max_mrpm) : 0;
        
        ret = pwm_set_pulse_dt(&pwm_motor_driver, w);
        if(ret != 0){
                return UNABLE_TO_SET_PWM_CHNL1;
        }

        return NOT_INITIALISED;
}

int speed_get(uint32_t* value){
        if(drv_initialised){
                *value = actual_mrpm; // TODO - get speed from encoder
                return SUCCESS;
        }

        return NOT_INITIALISED;
}

int motor_on(MotorDirection direction){
        if(drv_initialised){
                int ret;
                count_cycles = 0;
                old_count_cycles = 0;
                if(direction == BACKWARD){
                        ret = gpio_pin_set_dt(&set_dir_p1, 1);
                        if(ret != 0){
                                return UNABLE_TO_SET_GPIO;
                        }
                        ret = gpio_pin_set_dt(&set_dir_p2, 0);
                        if(ret != 0){
                                return UNABLE_TO_SET_GPIO;
                        }
                } else if(direction == FORWARD){
                        ret = gpio_pin_set_dt(&set_dir_p1, 0);
                        if(ret != 0){
                                return UNABLE_TO_SET_GPIO;
                        }
                        ret = gpio_pin_set_dt(&set_dir_p2, 1);
                        if(ret != 0){
                                return UNABLE_TO_SET_GPIO;
                        }
                }

                is_motor_on = true;
                return SUCCESS;
        }

        return NOT_INITIALISED;
}

int motor_off(void){
        if(drv_initialised){
                int ret = gpio_pin_set_dt(&set_dir_p1, 0);
                if(ret != 0){
                        return UNABLE_TO_SET_GPIO;
                }
                ret = gpio_pin_set_dt(&set_dir_p2, 0);
                if(ret != 0){
                        return UNABLE_TO_SET_GPIO;
                }
                is_motor_on = false;
                return SUCCESS;
        }

        return NOT_INITIALISED;
}

uint32_t speed_target_get(){
        return target_speed_mrpm;
}

#if defined(CONFIG_BOARD_NRF52840DONGLE_NRF52840)
void enter_boot(void){
        gpio_pin_configure_dt(&out_boot, GPIO_OUTPUT);
}
#endif

bool get_motor_off_on(void){
        return is_motor_on;
}

uint32_t get_current_max_speed(void){
        return max_mrpm;
}

uint64_t get_cycles_count_DEBUG(void){
        return count_cycles;
}

uint64_t get_time_cycles_count_DEBUG(void){
        return count_timer;
}

int32_t get_ret_DEBUG(void){
        return ret_debug;
}

uint32_t get_calc_speed_DEBUG(void){
        return speed_control;
}

DriverVersion get_driver_version(void){
        return driver_ver;
}
