// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2023 Maciej Baczmanski, Michal Kawiak, Jakub Mazur
// Copyright (c) 2016 Intel Corporation
#if defined(CONFIG_BT_SUPPORT)
// TODO - move define wrappers to CMakeLists. If whole is not to be compiled, make rule in CMAKE

#include "driver.h"

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/gatt.h>

static char ble_buff[8];

static ssize_t read_ble(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &ble_buff,
				 sizeof(ble_buff));
}

uint32_t convertToUint32(uint8_t *bytes)
{
	uint32_t result = 0;

	memcpy(&result, bytes, 4U);

	// swap endianness
	result = ((result << 8) & 0xFF00FF00) | ((result >> 8) & 0xFF00FF);
	return (result << 16) | (result >> 16);
}

static ssize_t write_ble(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset, uint8_t flag)
{
	uint8_t *data = (uint8_t *)buf;

	if (len == 1) {
		if (data[0] == 0) {
			init_pwm_motor_driver();
		}
	#if defined(CONFIG_BOARD_NRF52840DONGLE_NRF52840)

		else if (data[0] == 1) {
			enter_boot();
		}
	#endif
		else if (data[0] == 2) {
			uint32_t speed_tmp = speed_target_get();

			memcpy(ble_buff, &speed_tmp, sizeof(speed_tmp));

		} else if (data[0] == 3) {
			const struct DriverVersion tmp_ver = get_driver_version();

			memcpy(ble_buff, &tmp_ver, 2U);
		}
		return len;
	} else if (len == 5) {
		target_speed_set(convertToUint32(data+1U), CH0);
	}
	return len;
}


BT_GATT_SERVICE_DEFINE(hrs_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HRS),
	BT_GATT_CHARACTERISTIC(BT_UUID_HRS_BODY_SENSOR, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ,
			       read_ble, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_HRS_CONTROL_POINT, BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE,
			       NULL, write_ble, NULL),
);

#endif
