CC=gcc
CXX=g++
ifdef WITH_DEBUG
DEBUG=-DDEBUG
endif
CFLAGS=$(DEBUG) -g
LDFLAGS=-pthread -lrt

ALLSRC=mempool.c mempool_vary.c event_queue.c event_queue-test.c
ALLOBJ=$(ALLSRC:.c=.o)
TARGET=event_queue-test

all:$(TARGET)

event_queue-test:event_queue-test.o event_queue.o mempool.o
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
