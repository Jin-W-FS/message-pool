CC=gcc
CXX=g++
CFLAGS=-DDEBUG -g
LDFLAGS=-pthread

all:test

test:test.o mempool.o

.PHONY:all clean

clean:
	-rm test test.o mempool.o
