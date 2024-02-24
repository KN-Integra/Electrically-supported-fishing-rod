// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2023 Maciej Baczmanski, Michal Kawiak, Jakub Mazur

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include "button.h"
#include "driver.h"

static const struct gpio_dt_spec off_on_button = GPIO_DT_SPEC_GET(DT_ALIAS(off_on_button), gpios);

static struct gpio_callback off_on_cb;

static void cooldown_expired(struct k_work *work)
{
	ARG_UNUSED(work);

	int state = gpio_pin_get_dt(&off_on_button);

	if (state != 0) {
		motor_off(CH0);
	} else {
		motor_on(FORWARD, CH0);
	}
}

static K_WORK_DELAYABLE_DEFINE(cooldown_work, cooldown_expired);

static void off_on_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	k_work_reschedule(&cooldown_work, K_MSEC(15));
}

void off_on_button_init(void)
{
	int ret;

	ret = gpio_pin_configure_dt(&off_on_button, GPIO_INPUT | GPIO_PULL_DOWN);

	ret = gpio_pin_interrupt_configure_dt(&off_on_button, GPIO_INT_EDGE_BOTH);
	gpio_init_callback(&off_on_cb, off_on_callback, BIT(off_on_button.pin));
	gpio_add_callback(off_on_button.port, &off_on_cb);
}
