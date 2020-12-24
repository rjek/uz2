CC=gcc
CFLAGS=-O2 -D_POSIX_C_SOURCE=200809L

all: uz2

clean:
	rm -f uz2

uz2: uz2.c
	$(CC) $(CFLAGS) -o uz2 uz2.c -lz
