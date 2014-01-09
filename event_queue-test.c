#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include "event_queue.h"
#include "mempool.h"

int push_pop_test();
int thread_test(int nths);

int main(int argc, char* argv[])
{
	if (argc == 1) {
		return push_pop_test();
	} else {
		return thread_test(atoi(argv[1]));
	}
}

int push_pop_test()
{
	struct event_queue* eq = event_queue_new();
	char buff[80];
	while (fgets(buff, sizeof(buff), stdin)) {
		if (!isspace(buff[0])) {
			printf("push: %s", buff);
			event_queue_post(eq, strdup(buff));
		} else {
			void* p;
			if (event_queue_trywait(eq, &p) < 0) {
				printf("empty\n");
			} else {
				printf("pop: %s", (char*)p);
				free(p);
			}
		}
	}
	event_queue_del(eq);
	return 0;
}

struct thread_arg {
	pthread_t th;
	int id;
	struct event_queue* eq;
	memory_pool_t* pool;
};

int exit_all;
#define rand_t() (rand() % 3)
static void *thread_func(void *arg)
{
	struct thread_arg* p = arg;
	struct event_queue* eq = p->eq;
	memory_pool_t* pool = p->pool;
	while (!exit_all) {
		int rand_n = rand() % 200;
		if (rand_n > 100) {
			struct timespec ts;
			clock_gettime(CLOCK_REALTIME, &ts);
			ts.tv_sec += rand_t();
			void* data;
			if (event_queue_timedwait(eq, &data, &ts) < 0) {
				printf("[%d] empty\n", p->id);
			} else {
				printf("[%d] pop %d\n", p->id, *(int*)data);
				memory_pool_free(pool, data);
			}
		} else {
			int* data = memory_pool_alloc(pool);
			*data = rand_n;
			event_queue_post(eq, data);
			printf("[%d] push %d\n", p->id, *data);
		}
		sleep(rand_t());
	}
	return 0;
}

int thread_test(int nths)
{
	if (nths > 10) nths = 10;
	struct event_queue* eq = event_queue_new();
	memory_pool_t* pool = memory_pool_new(sizeof(int), 100, 1);
	struct thread_arg th_args[10];
	int i;
	for (i = 0; i < nths; i++) {
		th_args[i].id = i;
		th_args[i].eq = eq;
		th_args[i].pool = pool;
		pthread_create(&th_args[i].th, NULL, thread_func, &th_args[i]);
	}
	sleep(30);
	exit_all = 1;
	for (i = 0; i < nths; i++) {
		pthread_join(th_args[i].th, NULL);
	}
	/*  */
	void* data;
	while (event_queue_trywait(eq, &data) == 0) {
		printf("pop %d\n", *(int*)data);
		memory_pool_free(pool, data);
	}
	event_queue_del(eq);
	printf("pool: total=%d, allocated=%d\n", pool->nr_total, pool->nr_curr);
	memory_pool_del(pool);
	return 0;
}

