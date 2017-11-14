/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>

#include "memq.h"

inline memq_link_t *memq_peek(memq_t *memq, void **mem);

memq_link_t *memq_init(memq_t *memq, memq_link_t *link)
{
	/* head and tail pointer to the initial link node */
	memq->head = memq->tail = link;

	return link;
}

memq_link_t *memq_enqueue(memq_t *memq, memq_link_t *link, void *mem)
{
	/* make the current tail link node point to new link node */
	memq->tail->next = link;
	/* make the current tail mem point to the newly enqueued mem */
	memq->tail->mem = mem;

	/* increment the tail! */
	memq->tail = link;

	return link;
}

memq_link_t *memq_peek(memq_t *memq, void **mem)
{
	/* if head and tail are equal, then queue empty */
	if (memq->head == memq->tail) {
		return 0;
	}

	if (mem) {
		*mem = memq->head->mem;
	}

	return memq->head;
}

memq_link_t *memq_dequeue(memq_t *memq, void **mem)
{
	memq_link_t *link;

	/* use memq peek to get the link and mem */
	link = memq_peek(memq, mem);

	if (link) {
		/* increment the head to next link node */
		memq->head = link->next;
	}

	return link;
}

#if 0
u32_t memq_ut(void)
{
	void *head;
	void *tail;
	void *link;
	void *link_0[2];
	void *link_1[2];

	link = memq_init(&link_0[0], &head, &tail);
	if ((link != &link_0[0]) || (head != &link_0[0])
	    || (tail != &link_0[0])) {
		return 1;
	}

	link = memq_dequeue(tail, &head, NULL);
	if ((link) || (head != &link_0[0])) {
		return 2;
	}

	link = memq_enqueue(0, &link_1[0], &tail);
	if ((link != &link_1[0]) || (tail != &link_1[0])) {
		return 3;
	}

	link = memq_dequeue(tail, &head, NULL);
	if ((link != &link_0[0]) || (tail != &link_1[0])
	    || (head != &link_1[0])) {
		return 4;
	}

	return 0;
}
#endif
