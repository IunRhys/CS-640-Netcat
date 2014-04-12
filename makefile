CC=gcc
CFLAGS=-g -Wall -pthread
all: snc

snc: snc.o
	$(CC) $(CFLAGS) netcat.o -o snc
snc.o:
	$(CC) $(CFLAGS) -c netcat.c

clean:
	rm -rf *o snc
