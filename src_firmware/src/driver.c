#include <zephyr/drivers/pwm.h>
// #include <zephyr/timing/timing.h>

#include "driver.h"

static char* driver_ver = "0.1";

volatile static int32_t target_speed_mrpm = 0;
volatile static int32_t max_mrpm = 0;

volatile static int period = 0;

volatile static bool drv_initialised = false;

volatile static uint32_t actual_mrpm = 0;

static const struct pwm_dt_spec pwm_motor_driver = PWM_DT_SPEC_GET(DT_ALIAS(pwm_drv_ch1));

static const struct gpio_dt_spec set_dir_p1 = GPIO_DT_SPEC_GET(DT_ALIAS(set_dir_p1_ch1), gpios);
static const struct gpio_dt_spec out_boot = GPIO_DT_SPEC_GET(DT_ALIAS(enter_boot_p), gpios);

static const struct gpio_dt_spec enc_p1_ch1 = GPIO_DT_SPEC_GET(DT_ALIAS(get_enc_p1_ch1), gpios);
static const struct gpio_dt_spec enc_p2_ch1 = GPIO_DT_SPEC_GET(DT_ALIAS(get_enc_p2_ch1), gpios);
static struct gpio_callback enc1_cb;
static struct gpio_callback enc2_cb;


static uint64_t count_cycles = 0;
static uint64_t old_count_cycles = 0;

static uint64_t count_timer = 0;

struct k_timer my_timer;

// static uint32_t speed_delta = 0;

static int32_t ret_debug = 100;

static uint32_t last_calculated_speed = 0;

void update_speed_continuus(struct k_work *work)
{
    count_timer+=1;
    uint64_t diff = count_cycles - old_count_cycles;
    old_count_cycles = count_cycles;


    actual_mrpm = (diff*25)/4; //(diff*60)/(64*150) * 1000;
    int32_t speed_delta = target_speed_mrpm - actual_mrpm;

    uint8_t Kp_numerator = 4;
    uint8_t Kp_denominator = 10;

    last_calculated_speed = (uint32_t)((int32_t)last_calculated_speed + (int32_t)(Kp_numerator*speed_delta/Kp_denominator)); // increase or decrese speed each iteration by Kp * speed_delta

    ret_debug = speed_set_pwm(last_calculated_speed);
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
        period = pwm_motor_driver.period;
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

	ret = gpio_pin_configure_dt(&set_dir_p1, GPIO_OUTPUT_ACTIVE);

        if (0 != ret) {
                return UNABLE_TO_SET_GPIO;
        }

        if (!gpio_is_ready_dt(&out_boot)) {
                return GPIO_OUT_BOOT_NOT_READY;
	}


        ret = gpio_pin_configure_dt(&enc_p1_ch1, GPIO_INPUT);
        ret = gpio_pin_interrupt_configure_dt(&enc_p1_ch1, GPIO_INT_EDGE_BOTH);
        gpio_init_callback(&enc1_cb, enc_ch1_callback, BIT(enc_p1_ch1.pin));
        gpio_add_callback(enc_p1_ch1.port, &enc1_cb);


        ret = gpio_pin_configure_dt(&enc_p2_ch1, GPIO_INPUT);
        ret = gpio_pin_interrupt_configure_dt(&enc_p2_ch1, GPIO_INT_EDGE_BOTH);
        gpio_init_callback(&enc2_cb, enc_ch1_callback, BIT(enc_p2_ch1.pin));
        gpio_add_callback(enc_p2_ch1.port, &enc2_cb);


        k_timer_start(&my_timer, K_SECONDS(1), K_SECONDS(1));


        drv_initialised = true;

        return SUCCESS;
}

int target_speed_set(uint32_t value){
        target_speed_mrpm = value;
        return speed_set_pwm(value);
}

int speed_set_pwm(uint32_t value){
        int ret;
        last_calculated_speed = value;
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
                *value = actual_mrpm; // TODO - get speed from encoder
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
        return 0;
}

uint64_t get_cycles_count_DEBUG(void){
        return count_cycles;
}

uint64_t get_time_cycles_count_DEBUG(void){
        return count_timer;
}

uint32_t get_speed_delta_DEBUG(void){
        return 0;
}

int32_t get_ret_DEBUG(void){
        return ret_debug;
}

char* get_driver_version(void){
        return driver_ver;
}