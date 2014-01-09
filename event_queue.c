#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* #define NDEBUG */
#include <assert.h>

#include <errno.h>
#include <pthread.h>

#include "event_queue.h"

#define EVENT_QUEUE_USE_GLOBAL_MEMPOOL

#ifdef EVENT_QUEUE_USE_GLOBAL_MEMPOOL

#include "mempool.h"
/* a global thread-safe mempool */
static memory_pool_t* mempool_eqent;
static int mempool_eqent_init() {
	if (!mempool_eqent) {
		mempool_eqent = memory_pool_new(sizeof(struct event_queue_entry), 127, 1);
	}
	return mempool_eqent ? 0 : -1;
}
static struct event_queue_entry* mempool_eqent_alloc() {
	return memory_pool_alloc(mempool_eqent);
}
static void mempool_eqent_free(struct event_queue_entry* e) {
	memory_pool_free(mempool_eqent, e);
}
#define eqent_allocator_init()	mempool_eqent_init()
#define eqent_alloc()	mempool_eqent_alloc()
#define eqent_free(e)	mempool_eqent_free(e)

#else

#define eqent_allocator_init()	(0)
#define eqent_alloc()	malloc(sizeof(struct event_queue_entry))
#define eqent_free(e)	free(e)

#endif

/* unlocked push/pull */
static int event_queue_push_unsafe(struct event_queue* eq, void* event)
{
	struct event_queue_entry* e = eqent_alloc();
	if (!e) return -1;
	e->next = NULL;
	e->event = event;
	if (eq->tail == NULL) {
		assert(eq->head == NULL);
		eq->head = eq->tail = e;
	} else {
		assert(eq->head != NULL);
		eq->tail = eq->tail->next = e;
	}
	return 0;
}

static int event_queue_pop_unsafe(struct event_queue* eq, void** pevent)
{
	if (eq->head == NULL) {
		assert(eq->tail == NULL);
		return -1;
	}
	assert(eq->tail != NULL);
	struct event_queue_entry* e = eq->head;
	eq->head = eq->head->next;
	if (eq->head == NULL) {
		assert(eq->tail == e);
		eq->tail = NULL;
	}
	if (pevent) *pevent = e->event;
	eqent_free(e);
	return 0;
}

int event_queue_init(struct event_queue* eq)
{
	if (eqent_allocator_init() < 0) {
		return -1;
	}
	eq->head = eq->tail = NULL;
	pthread_cond_init(&eq->cond, NULL);
	pthread_mutex_init(&eq->lock, NULL);
	return 0;
}

void event_queue_destory(struct event_queue* eq)
{
	pthread_mutex_destroy(&eq->lock);
	pthread_cond_destroy(&eq->cond);
	void* event;
	while (event_queue_pop_unsafe(eq, &event) == 0) {
		/* memory in event is lost forever */
	}
}

struct event_queue* event_queue_new()
{
	struct event_queue* eq = malloc(sizeof(struct event_queue));
	if (!eq) return NULL;
	if (event_queue_init(eq) < 0) {
		free(eq); return NULL;
	}
	return eq;
}

void event_queue_del(struct event_queue* eq)
{
	event_queue_destory(eq);
	free(eq);
}

/* post event (void*) */
int event_queue_post(struct event_queue* eq, void* event)
{
	int ret;
	pthread_mutex_lock(&eq->lock);
	if ((ret = event_queue_push_unsafe(eq, event)) == 0) {
		pthread_cond_signal(&eq->cond);
	}
	pthread_mutex_unlock(&eq->lock);
	return ret;
}

int event_queue_timedwait(struct event_queue* eq, void** pevent, const struct timespec *abstime)
{
	int ret, pth_error = 0;
	pthread_mutex_lock(&eq->lock);
	while ((ret = event_queue_pop_unsafe(eq, pevent)) < 0 && pth_error != ETIMEDOUT) {
		pth_error = pthread_cond_timedwait(&eq->cond, &eq->lock, abstime);
	}
	pthread_mutex_unlock(&eq->lock);
	/* when while loop breaks, we have:
	 * 1) !(ret < 0) OR
	 * 2) pth_error == ETIMEDOUT */
	if (ret < 0) errno = pth_error;
	return ret;
}

int event_queue_wait(struct event_queue* eq, void** pevent)
{
	pthread_mutex_lock(&eq->lock);
	while (event_queue_pop_unsafe(eq, pevent) < 0) {
		pthread_cond_wait(&eq->cond, &eq->lock);
	}
	pthread_mutex_unlock(&eq->lock);
	return 0;
}

/* trywait as the sem_trywait, no wait on condition just check and return */
int event_queue_trywait(struct event_queue* eq, void** pevent)
{
	int ret;
	pthread_mutex_lock(&eq->lock);
	ret = event_queue_pop_unsafe(eq, pevent);
	pthread_mutex_unlock(&eq->lock);
	return ret;
}
