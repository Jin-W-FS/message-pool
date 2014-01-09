CC=gcc
CXX=g++
ifdef WITH_DEBUG
DEBUG=-DDEBUG
endif
CFLAGS=$(DEBUG) -g
LDFLAGS=-pthread

ALLSRC=mempool.c mempool_vary.c mempool-test.c mempool_vary-test.c 
ALLOBJ=$(ALLSRC:.c=.o)
TARGET=mempool-test mempool_vary-test

all:$(TARGET)

mempool-test:mempool-test.o mempool.o

mempool_vary-test:mempool_vary-test.o mempool_vary.o mempool.o

include $(ALLSRC:.c=.d)
%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

.PHONY:all clean

clean:
	-rm $(TARGET) $(ALLOBJ)
