#include <kernel.h>
#include <net/buf.h>
#include <soc.h>

#define TID0_STACK_SIZE 512
#define TID0_PRIO 5

#define TID1_STACK_SIZE 512
#define TID1_PRIO 5

#define BUF_COUNT 10

#define MEMQ 1
#if defined(MEMQ)
#include "../../../subsys/bluetooth/controller/util/mem.h"
#include "../../../subsys/bluetooth/controller/util/memq.h"

#define LINK_COUNT (BUF_COUNT + 1)
#define LINK_SIZE 8
u8_t MALIGN(4) pool_link[LINK_COUNT][LINK_SIZE];
void *pool_link_free = NULL;
void *q_head = NULL;
void *q_tail = NULL;
#else

K_FIFO_DEFINE(my_fifo);

struct data_item {
	void *fifo_reserved;
	void *data;
};

NET_BUF_POOL_DEFINE(pool_data, BUF_COUNT, sizeof(struct data_item), 0, NULL);
#endif


void tid0_entry(int unused0, int unused1, int unused2)
{
	printk("tid0_entry\n");

#if defined(MEMQ)
	while (1) {
		void *link, *link_ret;

		k_yield();

		NRF_GPIO->OUTSET = BIT(12);
		link = mem_acquire(&pool_link_free);
		if (!link) {
			NRF_GPIO->OUTCLR = BIT(12);
			continue;
		}

		link_ret = memq_enqueue(NULL, link, &q_tail);
		NRF_GPIO->OUTCLR = BIT(12);
		k_yield();

		if ((link_ret != link) || (q_tail != link)) {
			printk("memq_enqueue failed.\n");
			return;
		}
		printk("memq_enqueue success. (h: %p, t:%p)\n",
		       q_head, q_tail);
	}
#else
	while (1) {
		void *buf;

		k_yield();

		NRF_GPIO->OUTSET = BIT(12);
		buf = net_buf_alloc(&pool_data, K_FOREVER);
		if (!buf) {
			NRF_GPIO->OUTCLR = BIT(12);
			printk("net_buf_alloc failed.\n");
			return;
		}

		k_fifo_put(&my_fifo, buf);
		NRF_GPIO->OUTCLR = BIT(12);
		k_yield();

		printk("k_fifo_put success. (p: %p)\n", buf);
	}
#endif
}

void tid1_entry(int unused0, int unused1, int unused2)
{
	printk("tid1_entry\n");

#if defined(MEMQ)
	while (1) {
		void *link;

		k_yield();

		link = memq_dequeue(q_tail, &q_head, NULL);
		if (!link) {
			continue;
		}
		printk("memq_dequeue success. (h: %p, t: %p)\n",
		       q_head, q_tail);
	}
#else
	while (1) {
		void *buf;

		k_yield();

		buf = k_fifo_get(&my_fifo, K_FOREVER);
		printk("k_fifo_get success. (p: %p)\n", buf);
	}
#endif
}

void main(void)
{
#if defined(MEMQ)
	void *link, *link_ret;
#endif

	printk("main\n");

	NRF_GPIO->DIRSET = BIT(12);
	NRF_GPIO->OUTCLR = BIT(12);
#if defined(MEMQ)
	mem_init(pool_link, LINK_SIZE, LINK_COUNT, &pool_link_free);
	
	link = mem_acquire(&pool_link_free);
	if (!link) {
		printk("mem_acquire failed.\n");
		return;
	}

	link_ret = memq_init(link, &q_head, &q_tail);
	if ((link_ret != link) || (q_head != link)) {
		printk("memq_init failed.\n");
	}
#endif
}

K_THREAD_DEFINE(tid0, TID0_STACK_SIZE, tid0_entry, NULL, NULL, NULL,
		TID0_PRIO, 0, K_NO_WAIT);
K_THREAD_DEFINE(tid1, TID1_STACK_SIZE, tid1_entry, NULL, NULL, NULL,
		TID1_PRIO, 0, K_NO_WAIT);
