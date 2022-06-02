/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

static uint8_t mfg_data[] = { 0xff, 0xff, 0x00 };

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, 3),
};

void main(void)
{
	int err;
	int count = 1;

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}


	do {
		printk("Starting Broadcaster\n");
		printk("Bluetooth initialized\n");

		k_msleep(1000);

		printk("Sending advertising data: 0x%02X\n", mfg_data[2]);

		/* Start advertising */
		err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad),
				      NULL, 0);
		if (err) {
			printk("Advertising failed to start (err %d)\n", err);
			return;
		}

		k_msleep(1000);
		printk("Stop advertising\n");

		err = bt_le_adv_stop();
		if (err) {
			printk("Advertising failed to stop (err %d)\n", err);
			return;
		}

		mfg_data[2]++;

		k_sleep(K_SECONDS(1));
		printk("Disable Bluetooth\n");

		err = bt_disable();
		if (err) {
			printk("Bluetoot disable failed (err %d)\n", err);
			return;
		}

		printk("Enable Bluetooth\n");

		/* Initialize the Bluetooth Subsystem */
		err = bt_enable(NULL);
		if (err) {
			printk("Bluetooth init failed (err %d)\n", err);
			return;
		}

	} while (--count);
}
