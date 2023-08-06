#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/timing/timing.h>

#include "driver.h"

volatile static char* driver_ver = "0.1";

volatile static int32_t speed_mrpm = 0;
volatile static int32_t max_mrpm = 0;

volatile static int period = 0;

volatile static bool drv_initialised = false;

volatile static uint32_t last_time = 0;
volatile static uint32_t time_diff = 0;


volatile static uint32_t actual_mrpm = 0;

static const struct pwm_dt_spec pwm_motor_driver = PWM_DT_SPEC_GET(DT_ALIAS(pwm_drv_ch1));

static const struct gpio_dt_spec set_dir_p1 = GPIO_DT_SPEC_GET(DT_ALIAS(set_dir_p1_ch1), gpios);
static const struct gpio_dt_spec out_boot = GPIO_DT_SPEC_GET(DT_ALIAS(enter_boot_p), gpios);

static const struct gpio_dt_spec enc_p1_ch1 = GPIO_DT_SPEC_GET(DT_ALIAS(get_enc_p1_ch1), gpios);
static const struct gpio_dt_spec enc_p2_ch1 = GPIO_DT_SPEC_GET(DT_ALIAS(get_enc_p2_ch1), gpios);
static struct gpio_callback enc1_cb;
static struct gpio_callback enc2_cb;


static timing_t start_time, end_time;
static uint64_t total_cycles = 0;
static uint64_t total_ns = 0;

static uint64_t count_cycles = 0;


void enc_ch1_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins){
        count_cycles +=1;


        if(!drv_initialised){
                start_time = timing_counter_get();
        } else{
                end_time = timing_counter_get();

                total_cycles = timing_cycles_get(&start_time, &end_time);
                total_ns = timing_cycles_to_ns(total_cycles);

                uint64_t t = ((uint64_t)6E10) /total_ns;

                actual_mrpm = (uint32_t)(t * 64);

                start_time = timing_counter_get();
        }
}


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


        if (!gpio_is_ready_dt(&set_dir_p1)) {
                return GPIO_OUT_DIR_CNTRL_1_CHNL1_NOT_READY;
	}

	ret = gpio_pin_configure_dt(&set_dir_p1, GPIO_OUTPUT_ACTIVE);

        if (0 != ret) {
                return UNABLE_TO_SET_GPIO;
        }

        if (!gpio_is_ready_dt(&out_boot)) {
                return GPIO_OUT_BOOT_NOT_READY;
	}


        ret = gpio_pin_configure_dt(&enc_p1_ch1, GPIO_INPUT);
        ret = gpio_pin_interrupt_configure_dt(&enc_p1_ch1, GPIO_INT_EDGE_TO_ACTIVE);
        gpio_init_callback(&enc1_cb, enc_ch1_callback, BIT(enc_p1_ch1.pin));
        gpio_add_callback(enc_p1_ch1.port, &enc1_cb);


        ret = gpio_pin_configure_dt(&enc_p2_ch1, GPIO_INPUT);
        ret = gpio_pin_interrupt_configure_dt(&enc_p2_ch1, GPIO_INT_EDGE_TO_ACTIVE);
        gpio_init_callback(&enc2_cb, enc_ch1_callback, BIT(enc_p2_ch1.pin));
        gpio_add_callback(enc_p2_ch1.port, &enc2_cb);

        timing_init();
        timing_start();

        start_time = timing_counter_get();
        total_cycles = 0;



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
                
                total_cycles = 0;

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
                *value = actual_mrpm; // TODO - get speed from encoder
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

uint64_t get_current_time_DEBUG(void){
        end_time = timing_counter_get();

        total_cycles = timing_cycles_get(&start_time, &end_time);
        total_ns = timing_cycles_to_ns(total_cycles);

        start_time = timing_counter_get();

        return total_ns/1000000;
}

uint64_t get_cycles_count_DEBUG(void){
        return count_cycles;
}


char* get_driver_version(void){
        return driver_ver;
}