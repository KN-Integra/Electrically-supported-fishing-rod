// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2023 Maciej Baczmanski, Michal Kawiak, Jakub Mazur
// Copyright (c) 2016 Intel Corporation

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
