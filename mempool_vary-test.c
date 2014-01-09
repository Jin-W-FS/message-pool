#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include "mempool.h"
#include "mempool_vary.h"

int exit_all;

#define rand_t() (rand() % 3)
static void *thread_func(void *arg)
{
	vari_mempool_t* pool = arg;
	while (!exit_all) {
		void* e[4];
		int size = rand() % 35; /* 0 - 35 */
		int rand_n = rand() % 5;   /* 0 - 4 */
		int i;
		for (i = 0; i < rand_n; i++) {
			e[i] = vari_mempool_alloc(pool, size);
		}
		sleep(rand_t());
		for (i = 0; i < rand_n; i++) {
			vari_mempool_free(pool, e[i], size);
		}
		sleep(rand_t());
	}
	return 0;
}

#define NR_THS 3
int main(int argc, char *argv[])
{
	pthread_t ths[NR_THS];
	int i;
	srand(time(NULL));
	
	vari_mempool_t* pool = vari_mempool_new(24);

	for (i = 0; i < NR_THS; i++) {
		pthread_create(&ths[i], NULL, thread_func, pool);
	}

	sleep(30);
	exit_all = 1;
	
	for (i = 0; i < NR_THS; i++) {
		pthread_join(ths[i], NULL);
	}

	vari_mempool_del(pool);
	
	return 0;
}
