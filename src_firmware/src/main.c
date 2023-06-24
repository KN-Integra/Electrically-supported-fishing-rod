#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/console/console.h>
#include <zephyr/drivers/pwm.h>

#include <zephyr/drivers/gpio.h>

static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_ALIAS(pwm_drv));

static const struct gpio_dt_spec in1 = GPIO_DT_SPEC_GET(DT_ALIAS(in1), gpios);



void main(void)
{
	int ret;
	int period = pwm_led0.period;

	console_getline_init();

	if (!device_is_ready(pwm_led0.dev)) {
		printk("Error: PWM device %s is not ready\n",
		       pwm_led0.dev->name);
		return;
	}

	ret = pwm_set_pulse_dt(&pwm_led0, period/2);


	if(1 == ret){
		printk("sumfing was not yes");
	}


	if (!gpio_is_ready_dt(&in1)) {
		printk("sumfing was not yes");
	}

	ret = gpio_pin_configure_dt(&in1, GPIO_OUTPUT_ACTIVE);


	while (1) {
		char *s = console_getline();

		uint8_t code = s[0];

		if(0x21 == code){
			uint32_t new_RPM = (s[1] << 24) + (s[2] << 16) + (s[3] << 8) + s[4];
			uint32_t w = period * (uint64_t)new_RPM / 67000;

			ret = pwm_set_pulse_dt(&pwm_led0, w);
		}


		if(1 == ret){
			printk("sumfing was not yes");
		}

		k_sleep(K_SECONDS(1U));
	}
}
