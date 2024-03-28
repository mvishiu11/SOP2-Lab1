CC=gcc
CFLAGS=-std=gnu99 -Wall
LDFLAGS=-lrt

prog_mqueue: prog_mqueue.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

server: server.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

client: client.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

server_notify: server_notify.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)