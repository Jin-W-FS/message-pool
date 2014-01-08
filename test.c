#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include "mempool.h"

struct poolobj {
	int e;
};

int exit_all;

#define rand_t() (rand() % 3)
static void *thread_func(void *arg)
{
	memory_pool_t* pool = arg;
	while (!exit_all) {
		struct poolobj* e[5];
		bzero(e, sizeof(e)); assert(sizeof(e) == (sizeof(void*) * 5));
		int rand_n = rand() % 6;
		int i;
		for (i = 0; i < rand_n; i++) {
			e[i] = memory_pool_alloc(pool);
		}
		sleep(rand_t());
		for (i = 0; i < rand_n; i++) {
			memory_pool_free(pool, e[i]);
		}
		sleep(rand_t());
	}
	return 0;
}

int main(int argc, char *argv[])
{
	srand(time(NULL));
	pthread_t th1, th2, th3;
	memory_pool_t* pool = memory_pool_new(sizeof(struct poolobj), 5, 0);
	
	pthread_create(&th1, NULL, thread_func, pool);
	pthread_create(&th2, NULL, thread_func, pool);
	pthread_create(&th3, NULL, thread_func, pool);

	sleep(30);
	exit_all = 1;
	
	pthread_join(th1, NULL);
	pthread_join(th2, NULL);
	pthread_join(th3, NULL);

	printf("pool: total = %d, allocated = %d\n", pool->nr_total, pool->nr_curr);
	memory_pool_del(pool);

	return 0;
}
