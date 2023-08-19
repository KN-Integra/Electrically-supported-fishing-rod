#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>

#include <string.h>


void main(void)
{
	usb_enable(NULL);
	while (1) {

		k_sleep(K_SECONDS(1U));
	}
}
