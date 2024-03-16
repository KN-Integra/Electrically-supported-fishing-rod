// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2023 Maciej Baczmanski, Michal Kawiak, Jakub Mazur
// Copyright (c) 2016 Intel Corporation
#if defined(CONFIG_BT_SUPPORT)
// TODO - move define wrappers to CMakeLists. If whole is not to be compiled, make rule in CMAKE

#include "driver.h"
#include "storage.h"
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/gatt.h>
#include "return_codes.h"

#define UUID_GENERATOR(val)	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(val,		\
								       0x37de,		\
								       0x44d4,		\
								       0xbe45,		\
								       0xd1f1fd9385fd))


#define SERVICE_UUID			BT_UUID_DECLARE_16(0xffaa)
#define TX_CHAR_UUID_COMMAND		UUID_GENERATOR(0x6e006610)
#define RX_CHAR_UUID_TEMPL_LIST		UUID_GENERATOR(0x6e006620)
#define RX_CHAR_UUID_TEMPL_ACTIVE	UUID_GENERATOR(0x6e006621)
#define RX_CHAR_UUID_SPEED		UUID_GENERATOR(0x6e006622)
#define RX_CHAR_UUID_HW_VER		UUID_GENERATOR(0x6e006623)


//TODO - implement paging for more than 15 templates in the future

static ssize_t read_ble_template_list(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	ssize_t ret;
	char *buffer;
	uint8_t template_count = get_template_size();
	size_t frame_size = template_count * (sizeof(uint32_t) + CONFIG_TEMPLATE_NAME_SIZE) + 4;

	buffer = (char *)calloc(frame_size, sizeof(char));

	buffer[0] = frame_size;
	buffer[1] = 0; //page number
	buffer[2] = (char)template_count;
	buffer[3] = CONFIG_TEMPLATE_NAME_SIZE;

	(void)get_templates((struct Template *)&buffer[4]);

	ret = bt_gatt_attr_read(conn, attr, buf, len, offset, buffer,
				 frame_size);
	free(buffer);
	return ret;
}

static ssize_t read_ble_template_active(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	struct Template current;
	(void)get_current_template(&current);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &current,
				 sizeof(current));
}

static ssize_t read_ble_speed(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	uint32_t speed_tmp = speed_target_get();

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &speed_tmp,
				 sizeof(speed_tmp));
}

static ssize_t read_ble_hw_ver(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	struct DriverVersion tmp_ver = get_driver_version();

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &tmp_ver,
				 sizeof(tmp_ver));
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
	struct Template template;
	char *name;

	switch (data[0]) {
#if defined(CONFIG_BOARD_NRF52840DONGLE_NRF52840)
	case 0:
		enter_boot();
	break;
#endif
	case 1: // set raw speed
		(void)target_speed_set(convertToUint32(data+1U), CH0);
	break;
	case 2: // add template
		name = (char *)(data + 1U);
		strcpy(template.name, name);
		template.speed = convertToUint32(name + CONFIG_TEMPLATE_NAME_SIZE);
		set_template(template);

	break;
	case 3: // delete template
		name = (char *)(data+1U);
		(void)remove_template_by_name(name);
	break;
	case 4: //activate template
		return_codes_t ret;
		uint8_t out_temp; // TODO - necessary?

		ret = get_template_and_id_by_name(data+1U, &template, &out_temp);

		if (ret != SUCCESS) {
			break;
		}

		ret = target_speed_set(template.speed, CH0);

		if (ret != SUCCESS) {
			break;
		}
		set_current_template(template.name);
	break;
	}
	memset(data, 0, len * (sizeof(data[0])));
	return len;
}


BT_GATT_SERVICE_DEFINE(hrs_svc,
	BT_GATT_PRIMARY_SERVICE(SERVICE_UUID),
	BT_GATT_CHARACTERISTIC(RX_CHAR_UUID_TEMPL_LIST, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ,
			       read_ble_template_list, NULL, NULL),
	BT_GATT_CHARACTERISTIC(RX_CHAR_UUID_TEMPL_ACTIVE, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ,
			       read_ble_template_active, NULL, NULL),
	BT_GATT_CHARACTERISTIC(RX_CHAR_UUID_SPEED, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ,
			       read_ble_speed, NULL, NULL),
	BT_GATT_CHARACTERISTIC(RX_CHAR_UUID_HW_VER, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ,
			       read_ble_hw_ver, NULL, NULL),
	BT_GATT_CHARACTERISTIC(TX_CHAR_UUID_COMMAND, BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE,
			       NULL, write_ble, NULL),
);

#endif
