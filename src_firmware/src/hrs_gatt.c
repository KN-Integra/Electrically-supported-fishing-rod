#include "driver.h"
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/shell/shell.h>

#define LOG_LEVEL CONFIG_BT_HRS_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hrs);


#define GATT_PERM_READ_MASK     (BT_GATT_PERM_READ | \
				 BT_GATT_PERM_READ_ENCRYPT | \
				 BT_GATT_PERM_READ_AUTHEN)
#define GATT_PERM_WRITE_MASK    (BT_GATT_PERM_WRITE | \
				 BT_GATT_PERM_WRITE_ENCRYPT | \
				 BT_GATT_PERM_WRITE_AUTHEN)

#ifndef CONFIG_BT_HRS_DEFAULT_PERM_RW_AUTHEN
#define CONFIG_BT_HRS_DEFAULT_PERM_RW_AUTHEN 0
#endif
#ifndef CONFIG_BT_HRS_DEFAULT_PERM_RW_ENCRYPT
#define CONFIG_BT_HRS_DEFAULT_PERM_RW_ENCRYPT 0
#endif
#ifndef CONFIG_BT_HRS_DEFAULT_PERM_RW
#define CONFIG_BT_HRS_DEFAULT_PERM_RW 0
#endif

#define HRS_GATT_PERM_DEFAULT (						\
	CONFIG_BT_HRS_DEFAULT_PERM_RW_AUTHEN ?				\
	(BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_WRITE_AUTHEN) :	\
	CONFIG_BT_HRS_DEFAULT_PERM_RW_ENCRYPT ?				\
	(BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT) :	\
	(BT_GATT_PERM_READ | BT_GATT_PERM_WRITE))			\

static char hrs_blsc[160];

static void hrmc_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);

	bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);

	LOG_INF("HRS notifications %s", notif_enabled ? "enabled" : "disabled");
}

static ssize_t read_blsc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &hrs_blsc,
				 sizeof(hrs_blsc));
}

uint32_t convertToUint32(uint8_t *bytes) {
    uint32_t result = 0;
    memcpy(&result , bytes , 4U);
    return result;
}

static ssize_t write_blsc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset , uint8_t flag)
{
	uint8_t *data = (const uint8_t *)buf;
	if(len == 1){
		if(data[0] == 0){
			init_pwm_motor_driver(67000u);
		}
	#if defined(CONFIG_BOARD_NRF52840DONGLE_NRF52840)

		else if(data[0] == 1){
			enter_boot();
		}
	#endif
		else if (data[0] == 2){
			//static uint8_t tmp_hrm[4];
			uint32_t speed_tmp = speed_target_get();
			//bt_hrs_notify(tmp_hrm);
			memcpy(hrs_blsc, &speed_tmp, sizeof(speed_tmp));
			
		}else if(data[0] == 3){
			const DriverVersion tmp_ver = get_driver_version();
			memcpy(hrs_blsc, &tmp_ver, 2U);
		}else if(data[0] == 0xF0){
			//Test only
			bt_hrs_notify(10240);
		}
		return len;
	}else if(len == 5){
		target_speed_set(convertToUint32(data+1U));
	}

	return len;
}

/* Heart Rate Service Declaration */
BT_GATT_SERVICE_DEFINE(hrs_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HRS),
	BT_GATT_CHARACTERISTIC(BT_UUID_HRS_MEASUREMENT, BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(hrmc_ccc_cfg_changed,
		    HRS_GATT_PERM_DEFAULT),
	BT_GATT_CHARACTERISTIC(BT_UUID_HRS_BODY_SENSOR, BT_GATT_CHRC_READ,
			       HRS_GATT_PERM_DEFAULT & GATT_PERM_READ_MASK,
			       read_blsc, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_HRS_CONTROL_POINT, BT_GATT_CHRC_WRITE,
			       HRS_GATT_PERM_DEFAULT & GATT_PERM_WRITE_MASK,
			       NULL, write_blsc, NULL),
);

static int hrs_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

int bt_hrs_notify(uint32_t heartrate)
{
	int rc;
	static uint8_t hrm[4];

	memcpy(&hrm , &heartrate , 4U);
	rc = bt_gatt_notify(NULL, &hrs_svc.attrs[1], &hrm, sizeof(hrm));

	return rc == -ENOTCONN ? 0 : rc;
}

SYS_INIT(hrs_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
