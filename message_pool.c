#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <sys/eventfd.h>

#include "message_pool.h"

int msg_pool_init(msg_pool_t* mp, struct msg_pool_cfg* cfg)
{
	int i;
	for (i = 0; i < MSG_NR_CHANNELS; i++) {
		if (event_queue_init(&mp->equeue[i])) {
			return -1;
		}
		if (!cfg->use_event_fd[i]) {
			mp->efd[i] = -1;
		} else if ((mp->efd[i] = eventfd(0, EFD_NONBLOCK | EFD_SEMAPHORE)) < 0) {
			return -2;
		}
	}
	if ((mp->allocator = vari_mempool_new(&cfg->allocator_cfg)) == NULL) {
		return -3;
	}
	return 0;
}

void msg_pool_destory(msg_pool_t* mp)
{
	int i;
	for (i = 0; i < MSG_NR_CHANNELS; i++) {
		if (!(mp->efd[i] < 0)) {
			close(mp->efd[i]);
		}
		event_queue_destory(&mp->equeue[i]);
	}
	vari_mempool_del(mp->allocator);
}

msg_pool_t* msg_pool_new(struct msg_pool_cfg* cfg)
{
	msg_pool_t* mp = malloc(sizeof(msg_pool_t));
	if (mp && msg_pool_init(mp, cfg) < 0) {
		free(mp); mp = NULL;
	}
	return mp;
}

void msg_pool_del(msg_pool_t* mp)
{
	msg_pool_destory(mp);
	free(mp);
}

int msg_pool_efd_trypost(int fd)
{
	static const uint64_t val = 1;
	return write(fd, &val, sizeof(val));
}

int msg_pool_efd_trywait(int fd)
{
	uint64_t val;
	return read(fd, &val, sizeof(val));
}

int msg_pool_post(msg_pool_t* mp, int ch, void* msg)
{
	int ret;
	if ((ret = event_queue_post(&mp->equeue[ch], msg)) >= 0) {
		if (mp->efd[ch] >= 0) {
			if ((ret = msg_pool_efd_trypost(mp->efd[ch])) < 0) perror("msg_pool_efd_trypost");
		}
	}
	return ret;
}

