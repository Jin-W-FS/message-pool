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
	/* watcher_callback shall be called with watcher_data, nr_events,
	 * and change(+1/0/-1) each time after an action causing nr_events
	 * changed or a new watcher settled; calling is inside the queue
	 * lock so don't be time-consuming */
	int nr_events;
	void (*watcher_callback)(void* data, int nr_events, int change);
	void* watcher_data;
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

/* register a watcher */
void event_queue_register_watcher(struct event_queue* eq, void* data, void (*callback)(void*, int, int));

#endif
