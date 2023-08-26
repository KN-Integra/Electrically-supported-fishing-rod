#include <zephyr/kernel.h>

#include <zephyr/usb/usb_device.h>
#include "ble_gatt_service.h"
#include <string.h>

#include "driver.h"

#include <zephyr/drivers/uart.h>

void main(void)
{
	init_bt();
}
