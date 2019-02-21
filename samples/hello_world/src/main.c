/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <uart.h>

#define UART_DEVICE_NAME DT_UART_0_NAME
struct device *uart_dev;

const u8_t msg[] = "hello";

void poll_out(void)
{
	int i;
    	for (i = 0; i < ARRAY_SIZE(msg); i++) {
        	uart_poll_out(uart_dev, msg[i]);
    	}
}

void main(void)
{
#ifdef CONFIG_PRINTK
	printk("hello");
#else
	uart_dev = device_get_binding(UART_DEVICE_NAME);
	poll_out();
#endif
}
