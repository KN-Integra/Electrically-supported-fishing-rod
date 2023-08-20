
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include "bas_gatt.h"
#include "hrs_gatt.h"
// #include <zephyr/bluetooth/services/hrs.h>



static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_HRS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_BAS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_DIS_VAL))
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err 0x%02x)\n", err);
	} else {
		printk("Connected\n");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02x)\n", reason);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void bt_ready(void)
{
	int err;

	printk("Bluetooth initialized\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.cancel = auth_cancel,
};

void estimates_notify(uint8_t* estimate_buff , size_t num_of_bytes)
{
	bt_set_new_frame();
	uint8_t* ble_buffer = get_ble_buffer();
	int i = 0;
	while(true){
		if(i+16 > num_of_bytes){
			ble_buffer_copy_from_uint8_buffer(estimate_buff + i, num_of_bytes-i);
			bt_set_estimates(ble_buffer , num_of_bytes-i);
			break;
		}
		ble_buffer_copy_from_uint8_buffer(estimate_buff + i, 16);
		bt_set_estimates(ble_buffer , 16);
		i=i+16;
	}
	
}

// static void hrs_notify(void)
// {
// 	static uint8_t heartrate = 90U;

// 	/* Heartrate measurements simulation */
// 	heartrate++;
// 	if (heartrate == 160U) {
// 		heartrate = 90U;
// 	}

// 	bt_hrs_notify(heartrate);
// }

int bt_innit(void){
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return -1;
	}

	bt_ready();

	bt_conn_auth_cb_register(&auth_cb_display);
	return 0;
}




//TODO to be removed 
// void main_loop(void)
// {
	

// 	/* Implement notification. At the moment there is no suitable way
// 	 * of starting delayed work so we do it here
// 	 */
// 	while (1) {
// 		//k_sleep(K_SECONDS(1));

// 		/* Heartrate measurements simulation */
// 		hrs_notify();

// 		/* Battery level simulation */
// 		bas_notify();
// 	}
// }
