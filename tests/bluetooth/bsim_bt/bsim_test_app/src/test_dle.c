/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include <errno.h>

#include "kernel.h"
#include "bs_types.h"
#include "bs_tracing.h"
#include "time_machine.h"
#include "bstests.h"

#include <zephyr/types.h>
#include <zephyr.h>

#include <sys/byteorder.h>
#include <sys/printk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#define BT_LE_SCAN_CUSTOM BT_LE_SCAN_PARAM(BT_HCI_LE_SCAN_ACTIVE, \
					   BT_HCI_LE_SCAN_FILTER_DUP_DISABLE, \
					   0x04, \
					   0x04)
extern enum bst_result_t bst_result;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0x0d, 0x18),
};

static bool g_peripheral;

static int conn_update(struct bt_conn *conn)
{
	struct bt_le_conn_param param;
	int err;

	param.interval_min = 0x06;
	param.interval_max = 0x06;
	param.latency = 0;
	param.timeout = 10;

	err = bt_conn_le_param_update(conn, &param);
	if (err) {
		printk("conn update failed (err %d).", err);
	} else {
		printk("conn update initiated.");
	}

	return err;
}

static void write_cmd_cb(struct bt_conn *conn, void *user_data)
{
}

static int write_cmd(struct bt_conn *conn)
{
	u8_t data[244] = {0, };
	int err;

	err = bt_gatt_write_without_response_cb(conn, 0x0001, data,
						sizeof(data), false,
						write_cmd_cb, NULL);
	if (err) {
		printk("Write cmd failed (%d).\n", err);
	}

	return err;
}

static void mtu_exchange_cb(struct bt_conn *conn, u8_t err,
			    struct bt_gatt_exchange_params *params)
{
	printk("MTU exchange %s (%u)\n", err == 0U ? "successful" : "failed",
	       bt_gatt_get_mtu(conn));

	if (!g_peripheral) {
		//conn_update(conn);
	}
}

static struct bt_gatt_exchange_params mtu_exchange_params;

static int mtu_exchange(struct bt_conn *conn)
{
	int err;

	printk("MTU: %u\n", bt_gatt_get_mtu(conn));

	mtu_exchange_params.func = mtu_exchange_cb;

	err = bt_gatt_exchange_mtu(conn, &mtu_exchange_params);
	if (err) {
		printk("MTU exchange failed (err %d)", err);
	} else {
		printk("Exchange pending...");
	}

	return err;
}

static bool eir_found(struct bt_data *data, void *user_data)
{
	bt_addr_le_t *addr = user_data;
	int i;

	printk("[AD]: %u data_len %u\n", data->type, data->data_len);

	if (g_peripheral) {
		return true;
	}

	switch (data->type) {
	case BT_DATA_UUID16_SOME:
	case BT_DATA_UUID16_ALL:
		if (data->data_len % sizeof(u16_t) != 0U) {
			printk("AD malformed\n");
			return true;
		}

		for (i = 0; i < data->data_len; i += sizeof(u16_t)) {
			struct bt_uuid *uuid;
			u16_t u16;
			int err;

			memcpy(&u16, &data->data[i], sizeof(u16));
			uuid = BT_UUID_DECLARE_16(sys_le16_to_cpu(u16));
			if (bt_uuid_cmp(uuid, BT_UUID_HRS)) {
				continue;
			}

			err = bt_le_scan_stop();
			if (err) {
				printk("Stop LE scan failed (err %d)\n", err);
				continue;
			}

			(void)bt_conn_create_le(addr, BT_LE_CONN_PARAM_DEFAULT);

			return false;
		}
	}

	return true;
}

static void device_found(const bt_addr_le_t *addr, s8_t rssi, u8_t type,
			 struct net_buf_simple *ad)
{
	char dev[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, dev, sizeof(dev));

	printk("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i\n",
	       dev, type, ad->len, rssi);

	/* We're only interested in connectable events */
	if (type == BT_LE_ADV_IND || type == BT_LE_ADV_DIRECT_IND) {
		bt_data_parse(ad, eir_found, (void *)addr);
	}
}

static struct bt_conn *g_conn;

static void connected(struct bt_conn *conn, u8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		printk("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected: %s\n", addr);

	g_conn = conn;

	mtu_exchange(conn);
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);

	g_conn = NULL;

	bt_conn_unref(conn);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	if (g_peripheral) {
		err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
		if (err) {
			printk("Advertising failed to start (err %d)\n", err);
			return;
		}

		printk("Advertising successfully started\n");

		err = bt_le_scan_start(BT_LE_SCAN_CUSTOM, device_found);
		if (err) {
			printk("Scanning failed to start (err %d)\n", err);
			return;
		}

		printk("Scanning successfully started\n");
	} else {
		err = bt_le_scan_start(BT_LE_SCAN_CUSTOM, device_found);
		if (err) {
			printk("Scanning failed to start (err %d)\n", err);
			return;
		}

		printk("Scanning successfully started\n");
	}
}

static void test_main(void)
{
	int err;

	bs_trace_raw_time(3, "test main called\n");

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	bt_conn_cb_register(&conn_callbacks);

	while (1) {
		k_sleep(MSEC_PER_SEC);

		if (g_conn) {
			write_cmd(g_conn);
		}
	}
}

static void test_tick(bs_time_t HW_device_time)
{

	bst_result = Failed;
	bs_trace_error_line("test: FAILED\n");
}

static void test_init_peripheral(void)
{
	g_peripheral = true;
	bst_ticker_set_next_tick_absolute(20e6);
	bst_result = In_progress;
}

static void test_init_central(void)
{
	bst_ticker_set_next_tick_absolute(20e6);
	bst_result = In_progress;
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "test_dle_peripheral",
		.test_descr = "Tests DLE",
		.test_post_init_f = test_init_peripheral,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
	},
	{
		.test_id = "test_dle_central",
		.test_descr = "Tests DLE",
		.test_post_init_f = test_init_central,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_dle_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
