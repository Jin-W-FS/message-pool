#ifndef COMMON_MSG_EVENT_POOL_H
#define COMMON_MSG_EVENT_POOL_H

#include "event_queue.h"
#include "mempool_vary.h"

enum MSG_CHANNEL_T {
	MSG_CHANNEL_UPSTREAM,
	MSG_CHANNEL_DOWNSTREAM,
	MSG_NR_CHANNELS
};

typedef struct msg_pool_s
{
	vari_mempool_t* allocator;
	struct event_queue equeue[MSG_NR_CHANNELS];
	/* each channel has an unblocked eventfd/sem to be select()ed on */
	int efd[MSG_NR_CHANNELS];
} msg_pool_t;

struct msg_pool_cfg {
	int use_event_fd[MSG_NR_CHANNELS];
	struct vari_mempool_cfg allocator_cfg;
};

int msg_pool_init(msg_pool_t* mp, struct msg_pool_cfg* cfg);
void msg_pool_destory(msg_pool_t* mp);

msg_pool_t* msg_pool_new(struct msg_pool_cfg* cfg);
void msg_pool_del(msg_pool_t* mp);

#define msg_pool_alloc(mp, size)	vari_mempool_alloc(mp->allocator, size)
#define msg_pool_free(mp, msg, size)	vari_mempool_free(mp->allocator, msg, size)

/* if using efd, post on efd after an successful push */
int msg_pool_post(msg_pool_t* mp, int ch, void* msg);

#define msg_pool_wait(mp, ch, pmsg)	event_queue_wait(&mp->equeue[ch], (void**)pmsg)
#define msg_pool_timedwait(mp, ch, pmsg, abstime)	event_queue_wait(&mp->equeue[ch], (void**)pmsg, abstime)
#define msg_pool_trywait(mp, ch, pmsg)	event_queue_trywait(&mp->equeue[ch], (void**)pmsg)

/* use an unblocked event fd to select() on */
#define msg_pool_get_event_fd(mp, ch)	(mp->efd[ch])

/* wrapped non-blocking read/write 1 on eventfd */
int msg_pool_efd_trypost(int fd);
int msg_pool_efd_trywait(int fd);

#endif
