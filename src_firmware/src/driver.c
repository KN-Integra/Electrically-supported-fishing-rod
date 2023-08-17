#include <zephyr/drivers/pwm.h>
#include <string.h>

#include "driver.h"

static char* driver_ver = "0.1";

static ControlModes control_mode = SPEED;
static uint32_t current_position = 0u;
static uint32_t target_position = 0u;

static const uint32_t max_position = 3600u; // constant value that accuracy depends on
static uint32_t encoder_cpr = 9600u; // TODO - add some form of setter or as a part of init

volatile static uint32_t target_speed_mrpm = 0u;// Target set by user
volatile static uint32_t actual_mrpm = 0u; // actual speed calculated from encoder pins

volatile static uint32_t max_mrpm = 0u;// max rpm set by user in init (read from documentation)

static uint32_t last_calculated_speed = 0u; // temporary speed used for PID calculations


/// variables used for actual speed calculation based on encoder pin and timer interrupts
static uint64_t count_cycles = 0u;
static uint64_t old_count_cycles = 0u;
static uint64_t count_timer = 0u;
struct k_timer my_timer;
///

volatile static bool drv_initialised = false; // was init command sent?


static const struct pwm_dt_spec pwm_motor_driver = PWM_DT_SPEC_GET(DT_ALIAS(pwm_drv_ch1));

static const struct gpio_dt_spec set_dir_p1 = GPIO_DT_SPEC_GET(DT_ALIAS(set_dir_p1_ch1), gpios);
static const struct gpio_dt_spec out_boot = GPIO_DT_SPEC_GET(DT_ALIAS(enter_boot_p), gpios);

static const struct gpio_dt_spec enc_p1_ch1 = GPIO_DT_SPEC_GET(DT_ALIAS(get_enc_p1_ch1), gpios);
static const struct gpio_dt_spec enc_p2_ch1 = GPIO_DT_SPEC_GET(DT_ALIAS(get_enc_p2_ch1), gpios);

static const struct gpio_dt_spec enc_ch1_pins[2] = {enc_p1_ch1, enc_p2_ch1};

static struct gpio_callback enc_ch1_cb[2];


static int32_t ret_debug = 100;// DEBUG ONLY

int speed_pwm_set(uint32_t value){
        int ret;
        last_calculated_speed = value;
        if(drv_initialised){

                if(value > max_mrpm){
                        return DESIRED_VALUE_TO_HIGH;
                }

                uint64_t w_1 = pwm_motor_driver.period * (uint64_t)value;
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


void update_speed_and_position_continuus(struct k_work *work)
{
        count_timer+=1;
        uint64_t diff = count_cycles - old_count_cycles;
        old_count_cycles = count_cycles;


        actual_mrpm = (diff*25)/4; //(diff*60)/(64*150) * 1000;

        int32_t position_difference = (diff*max_position)/encoder_cpr;
        int32_t new_position = (int32_t)current_position + position_difference;
        if(new_position <=0){
                current_position = (uint32_t)(max_position + new_position);
        } else{
                current_position = ((uint32_t)new_position)%max_position;
        }


        //cutoff at max_mrpm TODO - figure out why it breaks the controller?
        // if(last_calculated_speed > max_mrpm){ 
        //         last_calculated_speed = max_mrpm;
        // }

        if(control_mode == SPEED){
                int32_t speed_delta = target_speed_mrpm - actual_mrpm;

                uint8_t Kp_numerator = 4;
                uint8_t Kp_denominator = 10;

                last_calculated_speed = (uint32_t)((int32_t)last_calculated_speed + (int32_t)(Kp_numerator*speed_delta/Kp_denominator)); // increase or decrese speed each iteration by Kp * speed_delta
                speed_pwm_set(last_calculated_speed);
        } else if(control_mode == POSITION){
                int32_t position_delta = target_position - current_position;
                if(position_delta <0){
                        position_delta = -position_delta;
                }
                if(position_delta >max_position/(36)){
                
                        // uint8_t Kp_numerator = 10;
                        // uint8_t Kp_denominator = 1;

                        // uint32_t scaled_to_speed = ((uint32_t)position_delta) * Kp_numerator * max_mrpm/(max_position*Kp_denominator);
                        // if(scaled_to_speed>max_mrpm){
                        //         scaled_to_speed = max_mrpm;
                        // }
                        // Code currently unused, 
                        // TODO - figure out actual scaling

                        ret_debug = speed_pwm_set(max_mrpm/3); // TODO - max_mrpm/3 is temporary!
                } else{
                        ret_debug = speed_pwm_set(0);
                }
                

        }
}

K_WORK_DEFINE(speed_update_work, update_speed_and_position_continuus);

void my_timer_handler(struct k_timer *dummy){
        k_work_submit(&speed_update_work);
}

K_TIMER_DEFINE(my_timer, my_timer_handler, NULL);

void enc_ch1_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins){
        count_cycles +=1;
}


int init_pwm_motor_driver(uint32_t speed_max_mrpm, ControlModes default_control_mode, uint32_t start_position){
        max_mrpm = speed_max_mrpm;
        control_mode = default_control_mode;
        current_position = start_position;
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

        for(int i = 0; i<2; ++i){
                ret = gpio_pin_configure_dt(&enc_ch1_pins[i], GPIO_INPUT);
                ret = gpio_pin_interrupt_configure_dt(&enc_ch1_pins[i], GPIO_INT_EDGE_BOTH);
                gpio_init_callback(&enc_ch1_cb[i], enc_ch1_callback, BIT(enc_ch1_pins[i].pin));
                gpio_add_callback(enc_ch1_pins[i].port, &enc_ch1_cb[i]);
                // TODO - ret error checking!!
        }

        k_timer_start(&my_timer, K_MSEC(100), K_MSEC(100));

        count_cycles = 0;
        old_count_cycles = 0;

        drv_initialised = true;

        return SUCCESS;
}

int target_speed_set(uint32_t value){
        if(drv_initialised){
                if(control_mode == SPEED){
                        target_speed_mrpm = value;
                        return speed_pwm_set(value);
                }
                return WRONG_MODE;
        }

        return NOT_INITIALISED;
        
}

int speed_get(uint32_t* value){
        if(drv_initialised){
                *value = actual_mrpm;
                return SUCCESS;
        }

        return NOT_INITIALISED;
}

int target_position_set(uint32_t new_target_position){
        if(drv_initialised){
                if(control_mode == POSITION){
                        if(new_target_position > max_position){
                                return DESIRED_VALUE_TO_HIGH;
                        }
                        target_position = new_target_position;
                        return SUCCESS;
                }
                return WRONG_MODE;
        }

        return NOT_INITIALISED;
}

int position_get(uint32_t* value){
        if(drv_initialised){
                *value = current_position;
                return SUCCESS;
        }

        return NOT_INITIALISED;
}


int mode_set(ControlModes new_mode){
        if(drv_initialised){
                control_mode = new_mode;
                return SUCCESS;
        }

        return NOT_INITIALISED;
        
}

int mode_get(ControlModes* value){
        if(drv_initialised){
                *value = control_mode;
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

uint64_t get_cycles_count_DEBUG(void){
        return count_cycles;
}

uint64_t get_time_cycles_count_DEBUG(void){
        return count_timer;
}

int32_t get_ret_DEBUG(void){
        return ret_debug;
}

char* get_driver_version(void){
        return driver_ver;
}


int get_control_mode_from_string(char* str_control_mode, ControlModes* ret_value){
        if(strcmp(str_control_mode, "speed") == 0){
                *ret_value = SPEED;
                return SUCCESS;
        } else if(strcmp(str_control_mode, "position") == 0){
                *ret_value = POSITION;
                return SUCCESS;
        } else{
                return WRONG_MODE;
        }
}
