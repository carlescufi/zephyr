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

#include <rpmsg_lite.h>

#include "common.h"

u8_t rx_msg_buf[MSG_MAX_SIZE];
u8_t tx_msg_buf[MSG_MAX_SIZE];

struct rpmsg_lite_endpoint *ept;
struct rpmsg_lite_instance *rpmsg_lite_dev;

int endpoint_cb(void *data, int len, unsigned long src, void *priv)
{
	memcpy((void *)rx_msg_buf, data, len);

	int status = rpmsg_lite_send(rpmsg_lite_dev,
				     ept,
				     (unsigned long int)M4_CORE_ADDR,
				     tx_msg_buf,
				     len,
				     RL_DONT_BLOCK);
	if (status < 0) {
		printk("send_message %u bytes failed with status %d\n", len, status);
	}

	return RL_SUCCESS;
}

void main(void)
{
	printk("\r\nRPMsg-Lite[remote] demo started\r\n");

	for (size_t i = 0; i < sizeof(tx_msg_buf); i++) {
		tx_msg_buf[i] = i;
	}

	rpmsg_lite_dev = rpmsg_lite_remote_init((void *)SHM_START_ADDR,
						0,
						0);
	if (!rpmsg_lite_dev) {
		printk("Could not create instance");
		return;
	}

	ept = rpmsg_lite_create_ept(rpmsg_lite_dev,
				    (unsigned long int)M0_CORE_ADDR,
				    endpoint_cb,
				    NULL);
	if (!ept) {
		printk("Could not create endpoint");
		return;
	}

	while (1) {
		k_sleep(100);
	}
}
