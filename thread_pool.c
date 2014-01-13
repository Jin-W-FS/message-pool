#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <errno.h>
#include <signal.h>
#include <pthread.h>

#include "thread_pool.h"

int thread_pool_init(struct thread_pool* tp)
{
	assert(tp->min_nr_threads <= tp->max_nr_threads);
	tp->nr = 0;
	if ((tp->th = malloc(sizeof(pthread_t) * tp->max_nr_threads)) == NULL) {
		return -1;
	}
	pthread_cond_init(&tp->cond, NULL);
	pthread_mutex_init(&tp->lock, NULL);
	return 0;
}

void thread_pool_destory(struct thread_pool* tp)
{
	assert(tp->nr == 0);	/* all threads popped */
	free(tp->th);
}

int thread_pool_push(struct thread_pool* tp)
{
	void* th_arg;
	int ret;
	th_arg = (tp->gain_thread_arg ? tp->gain_thread_arg(tp->nr) : NULL);
	if ((ret = pthread_create(&tp->th[tp->nr], NULL, tp->thread_fn, th_arg)) != 0) {
		fprintf(stderr, "thread-pool: pthread_create() = %d: %s\n", ret, strerror(ret));
		goto error;
	}
	tp->nr++;
	return 0;
error:
	errno = ret;
	return -1;
}

int thread_pool_pop(struct thread_pool* tp)
{
	int top = tp->nr - 1;
	int ret;
	void* th_ret;
	if (top < 0) {
		return -1;
	}
	pthread_cancel(tp->th[top]);
	if ((ret = pthread_join(tp->th[top], &th_ret)) != 0) {
		fprintf(stderr, "thread-pool: pthread_join() = %d: %s\n", ret, strerror(ret));
		goto error;
	}
	tp->nr--;
	if (tp->deal_thread_ret) {
		tp->deal_thread_ret(th_ret);
	}
	return 0;
error:
	errno = ret;
	return -1;
}

int thread_pool_command(struct thread_pool* tp, int cmd, int arg)
{
	/* check cmd/arg validation */
	switch (cmd) {
	case THPOOL_EXIT:
		arg = 0; break;
	case THPOOL_INC:
		/* if (arg > 0 && tp->nr >= tp->max_nr_threads) arg = 0; */
		/* if (arg < 0 && tp->nr <= tp->min_nr_threads) arg = 0; */
		if (arg == 0) return 0;
		break;
	default:
		return 0;
	}
	/* set */
	pthread_mutex_lock(&tp->lock);
	tp->cmd = cmd; tp->arg += arg;
	pthread_cond_signal(&tp->cond);
	pthread_mutex_unlock(&tp->lock);
	return 0;
}

int thread_pool_do_cmd(struct thread_pool* tp, int cmd, int arg)
{
	int i;
	switch (cmd) {
	case THPOOL_EXIT:
		return THPOOL_EXIT;
	case THPOOL_INC:
		if (arg > 0) {
			for (i = 0; i < arg && tp->nr < tp->max_nr_threads; i++) {
				thread_pool_push(tp);
			}
		} else {
			for (i = 0; i > arg && tp->nr > tp->min_nr_threads; i--) {
				thread_pool_pop(tp);
			}
		}
		printf("thread-pool: %+d threads, now %d\n", i, tp->nr);
		return 0;
	default:
		printf("thread-pool: unknown cmd %d\n", cmd);
		return -1;
	}
}

static void cleanup(void *arg) {
	pthread_mutex_t* lock = arg;
	pthread_mutex_unlock(lock);
}
void* thread_pool_start(void* arg)
{
	struct thread_pool* tp = arg;
	/* runtime allocated thread stack */
	if (thread_pool_init(tp) < 0) {
		perror("thread_pool_init");
		return NULL;
	}

	pthread_cleanup_push(cleanup, &tp->lock);
	/* create initial (a min number of) threads */
	while (tp->nr < tp->min_nr_threads) {
		if (thread_pool_push(tp) < 0) {
			printf("thread-pool: %c thread error, %s\n", '+', "exit");
			goto error;
		}
	}
	
	/* loop waiting control signals */
	while (1) {
		int cmd, arg;
		pthread_mutex_lock(&tp->lock);
		while (tp->cmd == THPOOL_NIL) {
			pthread_cond_wait(&tp->cond, &tp->lock);
		}
		cmd = tp->cmd; arg = tp->arg;
		tp->cmd = tp->arg = 0;
		pthread_mutex_unlock(&tp->lock);
		if (thread_pool_do_cmd(tp, cmd, arg) == THPOOL_EXIT) {
			break;
		}
	}

	printf("thread-pool: begin cancel all threads and exit\n");
	while (tp->nr > 0) {
		if (thread_pool_pop(tp) < 0) {
			printf("thread-pool: %c thread error, %s\n", '-', "exit");
			goto error;
		}
	}
	goto succeed;
error:
	tp->nr = 0;
succeed:
	thread_pool_destory(tp);
	pthread_cleanup_pop(1);
	return NULL;
}
