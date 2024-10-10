# Makefile for compiling write_server and read_server

CC = gcc
CFLAGS = -Wall
TARGETS = write_server read_server

all: $(TARGETS)

write_server: server.c server.h
	$(CC) $(CFLAGS) server.c -D WRITE_SERVER -o write_server

read_server: server.c server.h
	$(CC) $(CFLAGS) server.c -D READ_SERVER -o read_server

clean:
	rm -f $(TARGETS)

.PHONY: all clean
