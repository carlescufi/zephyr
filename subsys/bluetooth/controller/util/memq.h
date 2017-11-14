/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEMQ_H_
#define _MEMQ_H_

struct _memq_node {
	void *next;
	void *mem;
};

typedef struct _memq_node memq_link_t;

struct _memq {
	memq_link_t *head;
	memq_link_t * volatile tail;
};

typedef struct _memq memq_t;

memq_link_t *memq_init(memq_t *memq, memq_link_t *link);
memq_link_t *memq_enqueue(memq_t *memq, memq_link_t *link, void *mem);
memq_link_t *memq_peek(memq_t *memq, void **mem);
memq_link_t *memq_dequeue(memq_t *memq, void **mem);

#endif
