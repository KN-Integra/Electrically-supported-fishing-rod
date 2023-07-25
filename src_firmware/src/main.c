#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>

#include <string.h>


void main(void)
{

	while (1) {

		k_sleep(K_SECONDS(1U));
	}
}
