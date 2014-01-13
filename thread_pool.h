#ifndef AUTO_INCREASED_THREAD_POOL_H
#define AUTO_INCREASED_THREAD_POOL_H

#include <pthread.h>

struct thread_pool {
	/* cfg: thread functions */
	void* (*thread_fn)(void*);	  /* thread function */
	void* (*gain_thread_arg)(int id); /* gain arg of thread_fn */
	void  (*deal_thread_ret)(void*);  /* deal return value of thread_fn */
	/* cfg: pthread_t storage */	
	int min_nr_threads;
	int max_nr_threads;
	/* runtime inited values */
	/* storage */
	int nr;
	pthread_t* th;
	/* command */
	pthread_cond_t cond;
	pthread_mutex_t lock;
	int cmd, arg;
};

/* run as an other thread */
void* thread_pool_start(void* thread_pool);

#define THPOOL_NIL	0x0	/* null cmd */
#define THPOOL_EXIT	0x1
#define THPOOL_INC	0x2
/* #define THPOOL_DEC	0x3 */
/* #define THPOOL_SET	0x4 */
int thread_pool_command(struct thread_pool* tp, int cmd, int arg);

#endif

