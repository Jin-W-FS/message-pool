CC=gcc
CXX=g++
ifdef WITH_DEBUG
DEBUG=-DDEBUG
endif
CFLAGS=$(DEBUG) -g
LDFLAGS=-pthread -lrt

ALLSRC=mempool.c mempool_vary.c event_queue.c event_queue_watcher.c	\
	message_pool.c thread_pool.c message_pool-test.c
ALLOBJ=$(ALLSRC:.c=.o)
TARGET=message_pool-test

all:$(TARGET)

message_pool-test:$(ALLOBJ)
	$(CC) $^ $(LDFLAGS) -o $@

include $(ALLSRC:.c=.d)
%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

.PHONY:all clean

clean:
	-rm $(TARGET) $(ALLOBJ)
