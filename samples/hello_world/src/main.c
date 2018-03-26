/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <soc.h>
#include <arch/arm/cortex_m/exc.h>

void main(void)
{
	printk("%s: %s\n", CONFIG_ARCH, CONFIG_BOARD);
	
#if defined(CONFIG_ZERO_LATENCY_IRQS)
	printk("ZLI ON\n");
#else
	printk("ZLI OFF\n");
#endif
	printk("CONFIG_NUM_IRQ_PRIO_BITS: %d\n", CONFIG_NUM_IRQ_PRIO_BITS);
	printk("_IRQ_PRIO_OFFSET: %d\n", _IRQ_PRIO_OFFSET);
	printk("_EXC_IRQ_DEFAULT_PRIO: %d\n", _EXC_IRQ_DEFAULT_PRIO);
}
