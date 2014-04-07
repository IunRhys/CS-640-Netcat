CC=g++

all: snc

snc: snc.o
	$(CC) netcat.o -o snc
snc.o:
	$(CC)  -c netcat.c

clean:
	rm -rf *o snc
