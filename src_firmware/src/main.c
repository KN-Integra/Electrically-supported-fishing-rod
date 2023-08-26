#include <zephyr/kernel.h>

#include <zephyr/usb/usb_device.h>

#include <string.h>

#include "driver.h"
#include <zephyr/sys/printk.h>

#include <zephyr/drivers/uart.h>

void main(void)
{

	const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	uint32_t dtr = 0;

	if (usb_enable(NULL)) {
		return;
	}

	/* Poll if the DTR flag was set */
	while (!dtr) {
		uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
		/* Give CPU resources to low priority threads. */
		k_sleep(K_MSEC(100));
	}

	int console_init(void);
	printk("hello world");


	motor_off();
	while (1) {

		k_sleep(K_SECONDS(1U));
	}
}
