CC=gcc
CFLAGS=-std=gnu99 -Wall -pthread -lrt
LDFLAGS=-pthread -lrt 

l3-1: l3-1.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

l3-2_server: l3-2_server.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

l3-2_client: l3-2_client.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)