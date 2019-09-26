/*
 * Copyright (c) 2018, NXP
 * Copyright (c) 2019, Nordic Semiconductor ASA
 * Copyright (c) 2018, Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <drivers/ipm.h>
#include <sys/printk.h>
#include <device.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <init.h>
#include <gpio.h>

#include <rpmsg_lite.h>

#include "common.h"

u8_t rx_msg_buf[MSG_MAX_SIZE];
u8_t tx_msg_buf[MSG_MAX_SIZE];

struct rpmsg_lite_endpoint *ept;
struct rpmsg_lite_instance *rpmsg_lite_dev;

static struct device *gpio0;
static K_SEM_DEFINE(data_rx_sem, 0, 1);

static volatile int received_data;

int endpoint_cb(void *data, int len, unsigned long src, void *priv)
{
	gpio_pin_write(gpio0, TIMING_PIN, 1);
	memcpy((void *)rx_msg_buf, data, len);
	gpio_pin_write(gpio0, TIMING_PIN, 0);

	received_data = len;
	k_sem_give(&data_rx_sem);

	return RL_SUCCESS;
}

static unsigned int receive_message(void)
{
	k_sem_take(&data_rx_sem, K_FOREVER);
	return received_data;
}

void main(void)
{
	int status = 0;

	printk("\r\nRPMsg-Lite[master] demo started\r\n");

	gpio0 = device_get_binding("GPIO_0");
	if (!gpio0) {
		printk("Couldn't get GPIO binding");
		return;
	}
	gpio_pin_configure(gpio0, TIMING_PIN, GPIO_DIR_OUT);
	gpio_pin_write(gpio0, TIMING_PIN, 0);

	rpmsg_lite_dev = rpmsg_lite_master_init((void *)SHM_START_ADDR,
						SHM_SIZE,
						0,
						0);
	if (!rpmsg_lite_dev) {
		printk("Could not create instance");
		return;
	}

	ept = rpmsg_lite_create_ept(rpmsg_lite_dev,
				    (unsigned long int)M4_CORE_ADDR,
				    endpoint_cb,
				    NULL);
	if (!ept) {
		printk("Could not create endpoint");
		return;
	}

	for (size_t i = 0; i < sizeof(tx_msg_buf); i++) {
		tx_msg_buf[i] = i;
	}

	size_t msg_sizes[] = MSG_SIZES;

	for (uint8_t i = 0; i < ARRAY_SIZE(msg_sizes); i++) {
		/* Let the second core setup */
		k_sleep(100);

		size_t msg_size = msg_sizes[i];
		gpio_pin_write(gpio0, TIMING_PIN, 1);
		status = rpmsg_lite_send(rpmsg_lite_dev,
					 ept,
					 (unsigned long int)M0_CORE_ADDR,
					 tx_msg_buf,
					 msg_size,
					 RL_DONT_BLOCK);
		gpio_pin_write(gpio0, TIMING_PIN, 0);
		if (status < 0) {
			printk("send_message %u bytes failed with status %d\n", msg_size, status);
		}

		if (receive_message() != msg_size) {
			printk("wrong msg size\n");
			break;
		}

		for (size_t j = 0; j < msg_size; j++) {
			if (rx_msg_buf[j] != (j % 256)) {
				printk("Wrong data\n");
				break;
			}
			rx_msg_buf[j] = 0;
		}

		printk("Message %u bytes sent successfully\n", msg_size);
	}

	printk("RPMsg-Lite demo ended.\n");
}
