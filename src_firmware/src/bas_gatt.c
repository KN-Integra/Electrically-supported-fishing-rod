/** @file
 *  @brief GATT Battery Service
 */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>
#include <stdbool.h>
#include <zephyr/types.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include "bas_gatt.h"

#define LOG_LEVEL CONFIG_BT_BAS_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bas);

static uint8_t ble_gatt_buffer[20] = {0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
									0x20,0x20,0x20,0x20,0x20,0x20,0x21,0x21,0x21,0x21,
									};

static uint8_t ble_gatt_new_frame[20] = {0x4e, 0x45, 0x57, 0x20, 0x46, 0x52, 0x41, 0x4d, 0x45, 0x20
										,0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};

void ble_buffer_copy_estimates(uint32_t var1, uint32_t var2, uint32_t var3, uint32_t var4) {
    // Copy each uint32_t variable into the buffer
    memcpy(ble_gatt_buffer, &var1, sizeof(uint32_t));
    memcpy(ble_gatt_buffer + sizeof(uint32_t), &var2, sizeof(uint32_t));
    memcpy(ble_gatt_buffer + 2 * sizeof(uint32_t), &var3, sizeof(uint32_t));
    memcpy(ble_gatt_buffer + 3 * sizeof(uint32_t), &var4, sizeof(uint32_t));
}

void ble_buffer_copy_from_uint8_buffer(uint8_t* src_buffer, size_t src_length) {
    // Copy data from the uint8_t buffer into the ble_gatt_buffer
	//size_t copy_length = (src_length < sizeof(ble_gatt_buffer)) ? src_length : sizeof(ble_gatt_buffer);
    memcpy(ble_gatt_buffer, src_buffer, src_length);
}



static void blvl_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				       uint16_t value)
{
	ARG_UNUSED(attr);

	bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);

	LOG_INF("BAS Notifications %s", notif_enabled ? "enabled" : "disabled");
}

static ssize_t read_blvl(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	uint8_t* lvl8 = ble_gatt_buffer;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &lvl8,
				 sizeof(lvl8));
}

BT_GATT_SERVICE_DEFINE(bas,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
	BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, read_blvl, NULL,
			       &ble_gatt_buffer),
	BT_GATT_CCC(blvl_ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

static int bas_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

uint8_t* get_ble_buffer()
{
	return &ble_gatt_buffer[0];
}

int bt_set_new_frame(){
	int rc;

	rc = bt_gatt_notify(NULL, &bas.attrs[1], ble_gatt_new_frame, 20);

	return rc == -ENOTCONN ? 0 : rc;
}

int bt_set_estimates(uint8_t data[] ,  size_t num_of_bytes)
{
	int rc;

	rc = bt_gatt_notify(NULL, &bas.attrs[1], data, num_of_bytes);

	return rc == -ENOTCONN ? 0 : rc;
}

SYS_INIT(bas_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
