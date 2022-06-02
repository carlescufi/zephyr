/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>

static struct k_thread thread_data;
static K_KERNEL_STACK_DEFINE(thread_stack, 1024);

static K_FIFO_DEFINE(queue);

static void thread(void *p1, void *p2, void *p3)
{
	static struct k_poll_event events[1] = {
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_FIFO_DATA_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY,
						&queue,
						0x1),
	};

	printk("Thread Started\n");

	while (1) {
		int ev_count, err;

		events[0].state = K_POLL_STATE_NOT_READY;
		ev_count = 1;

		printk("Calling k_poll with %d events\n", ev_count);

		err = k_poll(events, ev_count, K_FOREVER);
		printk("k_poll return: %d\n", err);

		//process_events(events, ev_count);

		/* Make sure we don't hog the CPU if there's all the time
		 * some ready events.
		 */
		k_yield();
	}
}

static void create_thread(void)
{

	printk("main: creating thread\n");
	k_thread_create(&thread_data, thread_stack,
			K_KERNEL_STACK_SIZEOF(thread_stack),
			thread, NULL, NULL, NULL,
			K_PRIO_COOP(3),
			0, K_NO_WAIT);
	k_thread_name_set(&thread_data, "THREAD");
}

void main(void)
{
	printk("Hello World! %s\n", CONFIG_BOARD);

	create_thread();
	printk("main: sleeping\n");
	k_sleep(K_SECONDS(3));

	printk("main: aborting thread\n");

	k_thread_abort(&thread_data);
	
	printk("main: thread aborted\n");

	create_thread();

}
