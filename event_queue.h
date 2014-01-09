#ifndef MSG_QUEUE_WITH_MULTY_THREADED_WAIT_H
#define MSG_QUEUE_WITH_MULTY_THREADED_WAIT_H

#include <pthread.h>

struct event_queue_entry {
	struct event_queue_entry* next;
	void* event;
};

struct event_queue {
	struct event_queue_entry* head;
	struct event_queue_entry* tail;
	/* no need a sem, as data in the list is a best sem */
	pthread_cond_t cond;
	pthread_mutex_t lock;
	/* maybe an additional timer? but that can
	 * be solved by posting a timeout msg */
};

struct event_queue* event_queue_new();
void event_queue_del(struct event_queue* eq);

int event_queue_init(struct event_queue* eq);
void event_queue_destory(struct event_queue* eq);

/* wait for an event, stored in *pevent */
int event_queue_wait(struct event_queue* eq, void** pevent);
int event_queue_trywait(struct event_queue* eq, void** pevent);
int event_queue_timedwait(struct event_queue* eq, void** pevent, const struct timespec *abstime);

/* post event (void*) */
int event_queue_post(struct event_queue* eq, void* event);

#endif
