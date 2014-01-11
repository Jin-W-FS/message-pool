#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#include <errno.h>

#include "event_queue_watcher.h"

static inline void increase_dylimit(struct equeue_signal_watcher* watcher, int change)
{
	if (watcher->dylimit == watcher->dylimit_max) return;
	watcher->dylimit += watcher->dylimit_inc + (change - 1);
	if (watcher->dylimit_max >= 0 && watcher->dylimit > watcher->dylimit_max)
		watcher->dylimit = watcher->dylimit_max;
}

static inline void decrease_dylimit(struct equeue_signal_watcher* watcher, int change)
{
	if (watcher->dylimit == 0) return;
	watcher->dylimit += change; /* change should < 0 */
	if (watcher->dylimit < 0) watcher->dylimit = 0;
}

static inline void watch_dylimit(struct equeue_signal_watcher* watcher, int nr_events, int change)
{			 /* when fall back to original upper-limit: */
	if (watcher->limit[1] && nr_events < watcher->limit[1]) {
		decrease_dylimit(watcher, -1);
	}
}

static inline void watcher_signal(int signo, int val) 
{
	union sigval sval = { .sival_int = val };
	if (sigqueue(getpid(), signo, sval) < 0) {
		fprintf(stderr, "EQWatcher: sigqueue(%d, %d, %d): %s\n",
			getpid(), signo, val, strerror(errno));
	}
}

/* watch decrease below lower limit */
static void watch_dec(struct equeue_signal_watcher* watcher, int nr_events, int change)
{
	if (watcher->limit[0] < 0) return; /* no watch on limit[0] */
	int limit = watcher->limit[0];
	if (!(nr_events <= limit && limit < nr_events - change)) return;
	/* new <= limit < old (edge triggered) */
	watcher_signal(watcher->signo, -1);
}

static void watch_inc(struct equeue_signal_watcher* watcher, int nr_events, int change)
{
	if (watcher->limit[1] < 0) return; /* no watch on limit[1] */
	int limit = watcher->limit[1] + watcher->dylimit; /* dynamic increased limit */
	if (!(limit <= nr_events)) return;
	/* limit <= new (state triggered) */	
	increase_dylimit(watcher, change);
	watcher_signal(watcher->signo, nr_events);
}

/* change == 0 */
static void watch_init(struct equeue_signal_watcher* watcher, int nr_events, int change)
{
	watcher->dylimit = 0;
	int* limit = watcher->limit;
	if (0 <= limit[0] && nr_events <= limit[0]) { /* hit lower limit */
		watcher_signal(watcher->signo, -1);
		return;
	}
	if (0 <= limit[1] && limit[1] <= nr_events) { /* hit upper limit */
		increase_dylimit(watcher, nr_events - limit[1]);
		watcher_signal(watcher->signo, +1);
		return;
	}
}

void equeue_signal_watcher_cb(void* data, int nr_events, int change)
{
	if (change < 0) {
		watch_dec(data, nr_events, change);
	} else if (change > 0) {
		watch_inc(data, nr_events, change);
	} else {
		watch_init(data, nr_events, change);
	}
	watch_dylimit(data, nr_events, change);
}

